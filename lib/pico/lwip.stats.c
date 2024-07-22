// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdbool.h>

#include "lwip/stats.h"

#include "mlua/lwip.h"
#include "mlua/module.h"

#define SET_FIELD(subsys, name) \
    lua_pushinteger(ls, lwip_stats.subsys name); \
    lua_setfield(ls, -2, #name)

#define MOD_PROTO_STATS(subsys) \
int mod_ ## subsys(lua_State* ls) { \
    lua_createtable(ls, 0, 12); \
    mlua_lwip_lock(); \
    SET_FIELD(subsys., xmit); \
    SET_FIELD(subsys., recv); \
    SET_FIELD(subsys., fw); \
    SET_FIELD(subsys., drop); \
    SET_FIELD(subsys., chkerr); \
    SET_FIELD(subsys., lenerr); \
    SET_FIELD(subsys., memerr); \
    SET_FIELD(subsys., rterr); \
    SET_FIELD(subsys., proterr); \
    SET_FIELD(subsys., opterr); \
    SET_FIELD(subsys., err); \
    SET_FIELD(subsys., cachehit); \
    mlua_lwip_unlock(); \
    return 1; \
}

#define MOD_IGMP_STATS(subsys) \
int mod_ ## subsys(lua_State* ls) { \
    lua_createtable(ls, 0, 14); \
    mlua_lwip_lock(); \
    SET_FIELD(subsys., xmit); \
    SET_FIELD(subsys., recv); \
    SET_FIELD(subsys., drop); \
    SET_FIELD(subsys., chkerr); \
    SET_FIELD(subsys., lenerr); \
    SET_FIELD(subsys., memerr); \
    SET_FIELD(subsys., proterr); \
    SET_FIELD(subsys., rx_v1); \
    SET_FIELD(subsys., rx_group); \
    SET_FIELD(subsys., rx_general); \
    SET_FIELD(subsys., rx_report); \
    SET_FIELD(subsys., tx_join); \
    SET_FIELD(subsys., tx_leave); \
    SET_FIELD(subsys., tx_report); \
    mlua_lwip_unlock(); \
    return 1; \
}

#define TBL_MEM_STATS(field) \
    lua_createtable(ls, 0, 5); \
    mlua_lwip_lock(); \
    SET_FIELD(field, err); \
    SET_FIELD(field, avail); \
    SET_FIELD(field, used); \
    SET_FIELD(field, max); \
    SET_FIELD(field, illegal); \
    mlua_lwip_unlock()

#define SET_SUBSYSTEM(name) \
    lua_pushboolean(ls, true); \
    lua_setfield(ls, -2, #name);

void mod_SUBSYSTEMS(lua_State* ls, MLuaSymVal const* value) {
    lua_createtable(ls, 0, 16);
    SET_SUBSYSTEM(link);
    SET_SUBSYSTEM(etharp);
    SET_SUBSYSTEM(ip_frag);
    SET_SUBSYSTEM(ip);
    SET_SUBSYSTEM(icmp);
    SET_SUBSYSTEM(igmp);
    SET_SUBSYSTEM(udp);
    SET_SUBSYSTEM(tcp);
    SET_SUBSYSTEM(mem);
    SET_SUBSYSTEM(memp);
    SET_SUBSYSTEM(ip6);
    SET_SUBSYSTEM(icmp6);
    SET_SUBSYSTEM(ip6_frag);
    SET_SUBSYSTEM(mld6);
    SET_SUBSYSTEM(nd6);
    SET_SUBSYSTEM(mib2);
}

#if LINK_STATS
MOD_PROTO_STATS(link)
#endif

#if ETHARP_STATS
MOD_PROTO_STATS(etharp)
#endif

#if IPFRAG_STATS
MOD_PROTO_STATS(ip_frag)
#endif

#if IP_STATS
MOD_PROTO_STATS(ip)
#endif

#if ICMP_STATS
MOD_PROTO_STATS(icmp)
#endif

#if IGMP_STATS
MOD_IGMP_STATS(igmp)
#endif

#if UDP_STATS
MOD_PROTO_STATS(udp)
#endif

#if TCP_STATS
MOD_PROTO_STATS(tcp)
#endif

#if MEM_STATS
int mod_mem(lua_State* ls) {
    TBL_MEM_STATS(mem.);
    return 1;
}
#endif

int mod_mem_reset(lua_State* ls) {
#if MEM_STATS
    mlua_lwip_lock();
    lwip_stats.mem.max = lwip_stats.mem.used;
    mlua_lwip_unlock();
#endif
    return 0;
}

#if MEMP_STATS
int mod_memp(lua_State* ls) {
    lua_Unsigned index = luaL_checkinteger(ls, 1);
    if (index >= MEMP_MAX) return 0;
    mlua_lwip_lock();
    TBL_MEM_STATS(memp[index]->);
#if defined(LWIP_DEBUG) || MEMP_OVERFLOW_CHECK || LWIP_STATS_DISPLAY
    lua_pushstring(ls, memp_pools[index]->desc);
#else
    lua_pushnil(ls);
#endif
    mlua_lwip_unlock();
    return 2;
}
#endif

int mod_memp_reset(lua_State* ls) {
#if MEMP_STATS
    lua_Unsigned index = luaL_checkinteger(ls, 1);
    if (index < MEMP_MAX) {
        mlua_lwip_lock();
        lwip_stats.memp[index]->max = lwip_stats.memp[index]->used;
        mlua_lwip_unlock();
    }
#endif
    return 0;
}

#if IP6_STATS
MOD_PROTO_STATS(ip6)
#endif

#if ICMP6_STATS
MOD_PROTO_STATS(icmp6)
#endif

#if IP6_FRAG_STATS
MOD_PROTO_STATS(ip6_frag)
#endif

#if MLD6_STATS
MOD_IGMP_STATS(mld6);
#endif

#if ND6_STATS
MOD_PROTO_STATS(nd6)
#endif

#if MIB2_STATS
int mod_mib2(lua_State* ls) {
    lua_createtable(ls, 0, 48);
    mlua_lwip_lock();
    SET_FIELD(mib2., ipinhdrerrors);
    SET_FIELD(mib2., ipinaddrerrors);
    SET_FIELD(mib2., ipinunknownprotos);
    SET_FIELD(mib2., ipindiscards);
    SET_FIELD(mib2., ipindelivers);
    SET_FIELD(mib2., ipoutrequests);
    SET_FIELD(mib2., ipoutdiscards);
    SET_FIELD(mib2., ipoutnoroutes);
    SET_FIELD(mib2., ipreasmoks);
    SET_FIELD(mib2., ipreasmfails);
    SET_FIELD(mib2., ipfragoks);
    SET_FIELD(mib2., ipfragfails);
    SET_FIELD(mib2., ipfragcreates);
    SET_FIELD(mib2., ipreasmreqds);
    SET_FIELD(mib2., ipforwdatagrams);
    SET_FIELD(mib2., ipinreceives);
    SET_FIELD(mib2., tcpactiveopens);
    SET_FIELD(mib2., tcppassiveopens);
    SET_FIELD(mib2., tcpattemptfails);
    SET_FIELD(mib2., tcpestabresets);
    SET_FIELD(mib2., tcpoutsegs);
    SET_FIELD(mib2., tcpretranssegs);
    SET_FIELD(mib2., tcpinsegs);
    SET_FIELD(mib2., tcpinerrs);
    SET_FIELD(mib2., tcpoutrsts);
    SET_FIELD(mib2., udpindatagrams);
    SET_FIELD(mib2., udpnoports);
    SET_FIELD(mib2., udpinerrors);
    SET_FIELD(mib2., udpoutdatagrams);
    SET_FIELD(mib2., icmpinmsgs);
    SET_FIELD(mib2., icmpinerrors);
    SET_FIELD(mib2., icmpindestunreachs);
    SET_FIELD(mib2., icmpintimeexcds);
    SET_FIELD(mib2., icmpinparmprobs);
    SET_FIELD(mib2., icmpinsrcquenchs);
    SET_FIELD(mib2., icmpinredirects);
    SET_FIELD(mib2., icmpinechos);
    SET_FIELD(mib2., icmpinechoreps);
    SET_FIELD(mib2., icmpintimestamps);
    SET_FIELD(mib2., icmpintimestampreps);
    SET_FIELD(mib2., icmpinaddrmasks);
    SET_FIELD(mib2., icmpinaddrmaskreps);
    SET_FIELD(mib2., icmpoutmsgs);
    SET_FIELD(mib2., icmpouterrors);
    SET_FIELD(mib2., icmpoutdestunreachs);
    SET_FIELD(mib2., icmpouttimeexcds);
    SET_FIELD(mib2., icmpoutechos);
    SET_FIELD(mib2., icmpoutechoreps);
    mlua_lwip_unlock();
    return 1;
}
#endif

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(COUNTER_MAX, integer, (STAT_COUNTER)-1),
    MLUA_SYM_V(MEMP_COUNT, integer, MEMP_MAX),
    MLUA_SYM_P(SUBSYSTEMS, mod_),

#if LINK_STATS
    MLUA_SYM_F(link, mod_),
#else
    MLUA_SYM_V(link, boolean, false),
#endif
#if ETHARP_STATS
    MLUA_SYM_F(etharp, mod_),
#else
    MLUA_SYM_V(etharp, boolean, false),
#endif
#if IPFRAG_STATS
    MLUA_SYM_F(ip_frag, mod_),
#else
    MLUA_SYM_V(ip_frag, boolean, false),
#endif
#if IP_STATS
    MLUA_SYM_F(ip, mod_),
#else
    MLUA_SYM_V(ip, boolean, false),
#endif
#if ICMP_STATS
    MLUA_SYM_F(icmp, mod_),
#else
    MLUA_SYM_V(icmp, boolean, false),
#endif
#if IGMP_STATS
    MLUA_SYM_F(igmp, mod_),
#else
    MLUA_SYM_V(igmp, boolean, false),
#endif
#if UDP_STATS
    MLUA_SYM_F(udp, mod_),
#else
    MLUA_SYM_V(udp, boolean, false),
#endif
#if TCP_STATS
    MLUA_SYM_F(tcp, mod_),
#else
    MLUA_SYM_V(tcp, boolean, false),
#endif
#if MEM_STATS
    MLUA_SYM_F(mem, mod_),
#else
    MLUA_SYM_V(mem, boolean, false),
#endif
    MLUA_SYM_F(mem_reset, mod_),
#if MEMP_STATS
    MLUA_SYM_F(memp, mod_),
#else
    MLUA_SYM_V(memp, boolean, false),
#endif
    MLUA_SYM_F(memp_reset, mod_),
#if IP6_STATS
    MLUA_SYM_F(ip6, mod_),
#else
    MLUA_SYM_V(ip6, boolean, false),
#endif
#if ICMP6_STATS
    MLUA_SYM_F(icmp6, mod_),
#else
    MLUA_SYM_V(icmp6, boolean, false),
#endif
#if IP6_FRAG_STATS
    MLUA_SYM_F(ip6_frag, mod_),
#else
    MLUA_SYM_V(ip6_frag, boolean, false),
#endif
#if MLD6_STATS
    MLUA_SYM_F(mld6, mod_),
#else
    MLUA_SYM_V(mld6, boolean, false),
#endif
#if ND6_STATS
    MLUA_SYM_F(nd6, mod_),
#else
    MLUA_SYM_V(nd6, boolean, false),
#endif
#if MIB2_STATS
    MLUA_SYM_F(mib2, mod_),
#else
    MLUA_SYM_V(mib2, boolean, false),
#endif
};

MLUA_OPEN_MODULE(lwip.stats) {
    mlua_require(ls, "lwip", false);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
