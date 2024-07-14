// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_PICO_PICO_LWIPOPTS_H
#define _MLUA_LIB_PICO_PICO_LWIPOPTS_H

#define NO_SYS                      1
#define LWIP_NETCONN                0
#define LWIP_SOCKET                 0
#define MEM_LIBC_MALLOC             0
#define MEM_ALIGNMENT               4
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
#endif

// Subsystems.
#ifndef LWIP_UDP
#define LWIP_UDP                    0
#endif
#ifndef LWIP_TCP
#define LWIP_TCP                    0
#endif

// Heap memory.
#ifndef LWIP_MEM_SIZE
#define LWIP_MEM_SIZE               4000
#endif
#define MEM_SIZE                    LWIP_MEM_SIZE

// Memory pools.
#ifndef LWIP_MEMP_NUM_PBUF
#define LWIP_MEMP_NUM_PBUF              0
#endif
#define MEMP_NUM_PBUF                   LWIP_MEMP_NUM_PBUF

#ifndef LWIP_MEMP_NUM_RAW_PCB
#define LWIP_MEMP_NUM_RAW_PCB           4
#endif
#define MEMP_NUM_RAW_PCB                LWIP_MEMP_NUM_RAW_PCB

#ifndef LWIP_MEMP_NUM_UDP_PCB
#define LWIP_MEMP_NUM_UDP_PCB           4
#endif
#define MEMP_NUM_UDP_PCB                LWIP_MEMP_NUM_UDP_PCB

#ifndef LWIP_MEMP_NUM_TCP_PCB
#define LWIP_MEMP_NUM_TCP_PCB           5
#endif
#define MEMP_NUM_TCP_PCB                LWIP_MEMP_NUM_TCP_PCB

#ifndef LWIP_MEMP_NUM_TCP_PCB_LISTEN
#define LWIP_MEMP_NUM_TCP_PCB_LISTEN    5
#endif
#define MEMP_NUM_TCP_PCB_LISTEN         LWIP_MEMP_NUM_TCP_PCB_LISTEN

#ifndef LWIP_MEMP_NUM_TCP_SEG
#define LWIP_MEMP_NUM_TCP_SEG           32
#endif
#define MEMP_NUM_TCP_SEG                LWIP_MEMP_NUM_TCP_SEG

#ifndef LWIP_MEMP_NUM_ALTCP_PCB
#define LWIP_MEMP_NUM_ALTCP_PCB         LWIP_MEMP_NUM_TCP_PCB
#endif
#define MEMP_NUM_ALTCP_PCB              LWIP_MEMP_NUM_ALTCP_PCB

#ifndef LWIP_MEMP_NUM_REASSDATA
#define LWIP_MEMP_NUM_REASSDATA         5
#endif
#define MEMP_NUM_REASSDATA              LWIP_MEMP_NUM_REASSDATA

#ifndef LWIP_MEMP_NUM_FRAG_PBUF
#define LWIP_MEMP_NUM_FRAG_PBUF         15
#endif
#define MEMP_NUM_FRAG_PBUF              LWIP_MEMP_NUM_FRAG_PBUF

#ifndef LWIP_MEMP_NUM_ARP_QUEUE
#define LWIP_MEMP_NUM_ARP_QUEUE         10
#endif
#define MEMP_NUM_ARP_QUEUE              LWIP_MEMP_NUM_ARP_QUEUE

#ifndef LWIP_MEMP_NUM_IGMP_GROUP
#define LWIP_MEMP_NUM_IGMP_GROUP        10
#endif
#define MEMP_NUM_IGMP_GROUP             LWIP_MEMP_NUM_IGMP_GROUP

#ifndef LWIP_MEMP_NUM_ND6_QUEUE
#define LWIP_MEMP_NUM_ND6_QUEUE         20
#endif
#define MEMP_NUM_ND6_QUEUE              LWIP_MEMP_NUM_ND6_QUEUE

#ifndef LWIP_MEMP_NUM_MLD6_GROUP
#define LWIP_MEMP_NUM_MLD6_GROUP        4
#endif
#define MEMP_NUM_MLD6_GROUP             LWIP_MEMP_NUM_MLD6_GROUP

#ifndef LWIP_PBUF_POOL_SIZE
#define LWIP_PBUF_POOL_SIZE             16
#endif
#define PBUF_POOL_SIZE                  LWIP_PBUF_POOL_SIZE

// Statistics.
#ifndef LWIP_STATS
#define LWIP_STATS                  0
#endif
#if LWIP_STATS
#define LINK_STATS                  LWIP_LINK_STATS
#define ETHARP_STATS                LWIP_ETHARP_STATS
#define IPFRAG_STATS                LWIP_IPFRAG_STATS
#define IP_STATS                    LWIP_IP_STATS
#define ICMP_STATS                  LWIP_ICMP_STATS
#define IGMP_STATS                  LWIP_IGMP_STATS
#define UDP_STATS                   LWIP_UDP_STATS
#define TCP_STATS                   LWIP_TCP_STATS
#define MEM_STATS                   LWIP_MEM_STATS
#define MEMP_STATS                  LWIP_MEMP_STATS
#define SYS_STATS                   0
#define IP6_STATS                   LWIP_IP6_STATS
#define ICMP6_STATS                 LWIP_ICMP6_STATS
#define IP6_FRAG_STATS              LWIP_IP6_FRAG_STATS
#define MLD6_STATS                  LWIP_MLD6_STATS
#define ND6_STATS                   LWIP_ND6_STATS
#define MIB2_STATS                  LWIP_MIB2_STATS
#endif  // LWIP_STATS

#endif
