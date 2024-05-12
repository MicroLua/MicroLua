// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_PICO_PICO_LWIP_H
#define _MLUA_LIB_PICO_PICO_LWIP_H

#define NO_SYS                      1
#define LWIP_NETCONN                0
#define LWIP_SOCKET                 0
#define MEM_LIBC_MALLOC             0
#define MEM_ALIGNMENT               4
// TODO: Make some of the options below configurable
#define MEM_SIZE                    4000
#define MEMP_NUM_ARP_QUEUE          10
#define MEMP_NUM_TCP_SEG            32
#define PBUF_POOL_SIZE              24
// #define ETH_PAD_SIZE                2
#define TCP_MSS                     1460
#define TCP_WND                     (8 * TCP_MSS)
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) \
                                     / TCP_MSS)
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_TX_SINGLE_PBUF   1
#define LWIP_CHKSUM_ALGORITHM       3
#define LWIP_TCP_KEEPALIVE          1
#define DHCP_DOES_ARP_CHECK         0
#define LWIP_DHCP_DOES_ACD_CHECK    0

#ifndef NDEBUG
#define LWIP_DEBUG                  1
#define LWIP_STATS                  1
#define LWIP_STATS_DISPLAY          1
#endif

#ifndef LWIP_TCP
#define LWIP_TCP                    0
#endif
#ifndef LWIP_UDP
#define LWIP_UDP                    0
#endif

#ifndef LWIP_STATS
#define LWIP_STATS                  0
#endif
#ifndef LINK_STATS
#define LINK_STATS                  0
#endif
#ifndef MEM_STATS
#define MEM_STATS                   0
#endif
#ifndef MEMP_STATS
#define MEMP_STATS                  0
#endif
#ifndef SYS_STATS
#define SYS_STATS                   0
#endif

#endif
