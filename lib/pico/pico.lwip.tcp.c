// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include <stdbool.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "mlua/int64.h"
#include "mlua/lwip.h"
#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

// TODO: Check how micropython does it; it also uses lwIP

typedef struct TCP {
    struct tcp_pcb* pcb;
    MLuaEvent recv_event;
    MLuaEvent send_event;
    struct pbuf* recv_head;
    struct pbuf* recv_tail;
    u16_t recv_off;
    err_t err;
    bool connected: 1;
    bool rx_closed: 1;
} TCP;

static void handle_err(void* arg, err_t err) {
    if (arg == NULL) return;
    TCP* tcp = arg;
    tcp->err = err != ERR_OK ? err : ERR_CLSD;
    tcp->pcb = NULL;
    mlua_event_set(&tcp->recv_event);
    mlua_event_set(&tcp->send_event);
}

static char const TCP_name[] = "pico.lwip.TCP";

static inline TCP* check_TCP(lua_State* ls, int arg) {
    return luaL_checkudata(ls, arg, TCP_name);
}

static inline TCP* to_TCP(lua_State* ls, int arg) {
    return lua_touserdata(ls, arg);
}

#define lock_and_check_error(ls, tcp) do { \
    mlua_lwip_lock(); \
    if ((tcp)->err != ERR_OK) { \
        mlua_lwip_unlock(); \
        return mlua_lwip_push_err((ls), (tcp)->err); \
    } \
} while (false)

static int TCP_close(lua_State* ls) {
    TCP* tcp = check_TCP(ls, 1);
    mlua_lwip_lock();
    err_t err = tcp->err;
    if (err == ERR_OK) {
        tcp_arg(tcp->pcb, NULL);
        // Closing never fails <https://savannah.nongnu.org/bugs/?60757>.
        err = tcp_close(tcp->pcb);
        tcp->err = ERR_CLSD;
        tcp->pcb = NULL;
    }
    for (struct pbuf* p = tcp->recv_head; p != NULL;) {
        p = pbuf_free_header(p, p->len);
    }
    tcp->recv_head = tcp->recv_tail = NULL;
    mlua_lwip_unlock();
    mlua_event_disable(ls, &tcp->recv_event);
    mlua_event_disable(ls, &tcp->send_event);
    return mlua_lwip_push_result(ls, err);
}

static int TCP_shutdown(lua_State* ls) {
    TCP* tcp = check_TCP(ls, 1);
    bool rx = mlua_to_cbool(ls, 2);
    bool tx = mlua_to_cbool(ls, 3);
    lock_and_check_error(ls, tcp);
    // Shutting down both directions is the same as close()
    if (rx && tx) tcp_arg(tcp->pcb, NULL);
    err_t err = tcp_shutdown(tcp->pcb, rx, tx);
    if (rx && tx) {
        tcp->err = ERR_CLSD;
        tcp->pcb = NULL;
    }
    mlua_lwip_unlock();
    return mlua_lwip_push_result(ls, err);
}

static int TCP_bind(lua_State* ls) {
    TCP* tcp = check_TCP(ls, 1);
    ip_addr_t const* addr = mlua_check_IPAddr(ls, 2);
    u16_t port = luaL_checkinteger(ls, 3);
    lock_and_check_error(ls, tcp);
    err_t err = tcp_bind(tcp->pcb, addr, port);
    mlua_lwip_unlock();
    return mlua_lwip_push_result(ls, err);
}

static int TCP_listen(lua_State* ls) {
    TCP* tcp = check_TCP(ls, 1);
    // TODO
    (void)tcp;
    return 0;
}

static int TCP_accept(lua_State* ls) {
    TCP* tcp = check_TCP(ls, 1);
    // TODO
    (void)tcp;
    return 0;
}

static err_t handle_connected(void* arg, struct tcp_pcb* pcb, err_t err) {
    if (arg == NULL) return tcp_abort(pcb), ERR_ABRT;
    TCP* tcp = arg;
    if (err != ERR_OK) {
        // The documentation doesn't say if the PCB has been freed or not. It
        // does say that this never happens.
        mlua_abort("BUG: pico.lwip.tcp: handle_connected: err != ERR_OK\n");
    }
    tcp->connected = true;
    mlua_event_set(&tcp->recv_event);
    return ERR_OK;
}

static int connect_loop(lua_State* ls, bool timeout) {
    TCP* tcp = to_TCP(ls, 1);
    lock_and_check_error(ls, tcp);
    bool connected = tcp->connected;
    mlua_lwip_unlock();
    if (!connected) {
        if (!timeout) return -1;
        return mlua_lwip_push_err(ls, ERR_TIMEOUT);
    }
    return lua_pushboolean(ls, true), 1;
}

static int TCP_connect(lua_State* ls) {
    TCP* tcp = check_TCP(ls, 1);
    ip_addr_t const* addr = mlua_check_IPAddr(ls, 2);
    u16_t port = luaL_checkinteger(ls, 3);
    lua_settop(ls, 4);  // Ensure deadline is set
    if (!mlua_event_enable(ls, &tcp->recv_event)) {
        return luaL_error(ls, "TCP: busy");
    }
    if (!mlua_event_enable(ls, &tcp->send_event)) {
        mlua_event_disable(ls, &tcp->recv_event);
        return luaL_error(ls, "TCP: busy");
    }
    lock_and_check_error(ls, tcp);
    err_t err = tcp_connect(tcp->pcb, addr, port, &handle_connected);
    mlua_lwip_unlock();
    if (err != ERR_OK) return mlua_lwip_push_err(ls, err);
    lua_rotate(ls, 2, 1);
    lua_settop(ls, 2);
    return mlua_event_wait(ls, &tcp->recv_event, 0, &connect_loop, 2);
}

static err_t handle_sent(void* arg, struct tcp_pcb* pcb, u16_t len) {
    if (arg == NULL) return tcp_abort(pcb), ERR_ABRT;
    TCP* tcp = arg;
    mlua_event_set(&tcp->send_event);
    return ERR_OK;
}

static err_t handle_poll(void* arg, struct tcp_pcb* pcb) {
    if (arg == NULL) return tcp_abort(pcb), ERR_ABRT;
    TCP* tcp = arg;
    mlua_event_set(&tcp->send_event);
    return ERR_OK;
}

static int send_loop(lua_State* ls, bool timeout) {
    TCP* tcp = to_TCP(ls, 1);
    int index = lua_tointeger(ls, -2);
    lua_Unsigned offset = lua_tointeger(ls, -1);
    int last = lua_gettop(ls) - 3;
    while (index <= last) {
        MLuaBuffer buf;
        if (!mlua_get_ro_buffer(ls, index, &buf) || buf.vt != NULL) {
            return luaL_typeerror(ls, index, "string or raw buffer");
        }
        while (offset < buf.size) {
            lock_and_check_error(ls, tcp);
            u16_t sz = tcp_sndbuf(tcp->pcb);
            if (sz > buf.size) sz = buf.size;
            if (sz > 0) {
                err_t err = tcp_write(
                    tcp->pcb, buf.ptr + offset, sz,
                    TCP_WRITE_FLAG_COPY
                    | (index < last ? TCP_WRITE_FLAG_MORE : 0));
                if (err == ERR_OK) {  // Data written, update pointers
                    offset += sz;
                } else if (err == ERR_MEM) {  // No room
                    sz = 0;
                } else {  // Write failed, abort
                    mlua_lwip_unlock();
                    return mlua_lwip_push_err(ls, err);
                }
            }
            mlua_lwip_unlock();
            if (sz == 0) {  // Nothing written, wait for event
                lua_pop(ls, 2);
                lua_pushinteger(ls, index);
                lua_pushinteger(ls, offset);
                return -1;
            }
        }
        ++index;
        offset = 0;
    }
    // TODO: Make the call to tcp_output() optional
    lock_and_check_error(ls, tcp);
    err_t err = tcp_output(tcp->pcb);
    mlua_lwip_unlock();
    return mlua_lwip_push_result(ls, err);
}

static int TCP_send(lua_State* ls) {
    TCP* tcp = check_TCP(ls, 1);
    // TODO: Support WRITE_FLAG_MORE
    // TODO: Support !WRITE_FLAG_COPY by waiting for "sent"
    if (!mlua_is_time(ls, -1)) lua_pushnil(ls);  // deadline
    lua_pushinteger(ls, 2);  // index
    lua_pushinteger(ls, 0);  // offset
    return mlua_event_wait(ls, &tcp->send_event, 0, &send_loop, -3);
}

static err_t handle_recv(void* arg, struct tcp_pcb* pcb, struct pbuf* p,
                         err_t err) {
    if (arg == NULL) {
        if (p != NULL) pbuf_free(p);
        return tcp_abort(pcb), ERR_ABRT;
    }
    TCP* tcp = arg;
    if (p != NULL) {
        if (p->tot_len == 0) {
            pbuf_free(p);
            return ERR_OK;
        }
        struct pbuf* t = tcp->recv_tail;
        if (t != NULL) t->next = p; else tcp->recv_head = p;
        while (p->next != NULL) p = p->next;
        tcp->recv_tail = p;
    } else if (err == ERR_OK) {
        tcp->rx_closed = true;
    } else {
        // The documentation doesn't say if the PCB has been freed or not. From
        // the sources, this should never happen.
        mlua_abort("BUG: pico.lwip.tcp: handle_recv: err != ERR_OK");
    }
    mlua_event_set(&tcp->recv_event);
    return ERR_OK;
}

static int recv_loop(lua_State* ls, bool timeout) {
    TCP* tcp = to_TCP(ls, 1);
    lua_Integer len = lua_tointeger(ls, 2);
    luaL_Buffer buf = {.n = 0};
    mlua_lwip_lock();  // Report error after data
    struct pbuf* h = tcp->recv_head;
    u16_t off = tcp->recv_off;
    while (h != NULL && len > 0) {
        if (luaL_bufflen(&buf) == 0) luaL_buffinit(ls, &buf);
        u16_t sz = h->len - off;
        if (len < sz) sz = len;
        pbuf_copy_partial(h, luaL_prepbuffsize(&buf, sz), sz, off);
        luaL_addsize(&buf, sz);
        len -= sz;
        if (off + sz == h->len) {
            h = pbuf_free_header(h, h->len);
            off = 0;
            mlua_event_set(&tcp->send_event);
        } else {
            off += sz;
        }
        if (tcp->pcb != NULL) tcp_recved(tcp->pcb, sz);
    }
    tcp->recv_head = h;
    if (h == NULL) tcp->recv_tail = NULL;
    tcp->recv_off = off;
    err_t err = tcp->err;
    bool rx_closed = tcp->rx_closed;
    mlua_lwip_unlock();
    if (luaL_bufflen(&buf) > 0) return luaL_pushresult(&buf), 1;
    if (rx_closed) return lua_pushliteral(ls, ""), 1;
    if (err != ERR_OK) return mlua_lwip_push_err(ls, err);
    if (timeout) return mlua_lwip_push_err(ls, ERR_TIMEOUT);
    return -1;
}

static int TCP_recv(lua_State* ls) {
    TCP* tcp = check_TCP(ls, 1);
    lua_Integer len = luaL_checkinteger(ls, 2);
    if (len <= 0) return lua_pushliteral(ls, ""), 1;
    lua_settop(ls, 3);  // Ensure deadline is set
    return mlua_event_wait(ls, &tcp->recv_event, 0, &recv_loop, 3);
}

static int TCP_set_prio(lua_State* ls) {
    TCP* tcp = check_TCP(ls, 1);
    u8_t prio = luaL_checkinteger(ls, 2);
    mlua_lwip_lock();
    if (tcp->pcb != NULL) tcp_setprio(tcp->pcb, prio);
    mlua_lwip_unlock();
    return 0;
}

#define TCP_write TCP_send
#define TCP_read TCP_recv

MLUA_SYMBOLS(TCP_syms) = {
    MLUA_SYM_F(close, TCP_),
    MLUA_SYM_F(shutdown, TCP_),
    MLUA_SYM_F(bind, TCP_),
    MLUA_SYM_F(listen, TCP_),
    MLUA_SYM_F(accept, TCP_),
    MLUA_SYM_F(connect, TCP_),
    MLUA_SYM_F(send, TCP_),
    MLUA_SYM_F(write, TCP_),
    MLUA_SYM_F(recv, TCP_),
    MLUA_SYM_F(read, TCP_),
    MLUA_SYM_F(set_prio, TCP_),
};

#define TCP___close TCP_close
#define TCP___gc TCP_close

MLUA_SYMBOLS_NOHASH(TCP_syms_nh) = {
    MLUA_SYM_F_NH(__close, TCP_),
    MLUA_SYM_F_NH(__gc, TCP_),
};

static int mod_new(lua_State* ls) {
    u8_t type = luaL_optinteger(ls, 1, IPADDR_TYPE_ANY);
    TCP* tcp = lua_newuserdatauv(ls, sizeof(TCP), 0);
    memset(tcp, 0, sizeof(*tcp));
    luaL_getmetatable(ls, TCP_name);
    lua_setmetatable(ls, -2);
    mlua_lwip_lock();
    tcp->pcb = tcp_new_ip_type(type);
    if (tcp->pcb == NULL) {
        mlua_lwip_unlock();
        return mlua_lwip_push_err(ls, ERR_MEM);
    }
    tcp_arg(tcp->pcb, tcp);
    tcp_err(tcp->pcb, &handle_err);
    tcp_recv(tcp->pcb, &handle_recv);
    tcp_sent(tcp->pcb, &handle_sent);
    tcp_poll(tcp->pcb, &handle_poll, 2);  // Interval: 1s
    mlua_lwip_unlock();
    return 1;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(WRITE_FLAG_COPY, integer, TCP_WRITE_FLAG_COPY),
    MLUA_SYM_V(WRITE_FLAG_MORE, integer, TCP_WRITE_FLAG_MORE),
    MLUA_SYM_V(PRIO_MIN, integer, TCP_PRIO_MIN),
    MLUA_SYM_V(PRIO_NORMAL, integer, TCP_PRIO_NORMAL),
    MLUA_SYM_V(PRIO_MAX, integer, TCP_PRIO_MAX),

    MLUA_SYM_F(new, mod_),
};

MLUA_OPEN_MODULE(pico.lwip.tcp) {
    mlua_thread_require(ls);
    mlua_require(ls, "mlua.int64", false);
    mlua_require(ls, "pico.lwip", false);
    mlua_require(ls, "pico.lwip.pbuf", false);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);

    // Create the TCP class.
    mlua_new_class(ls, TCP_name, TCP_syms, TCP_syms_nh);
    lua_pop(ls, 1);
    return 1;
}
