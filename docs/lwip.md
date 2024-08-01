<!-- Copyright 2024 Remy Blank <remy@c-space.org> -->
<!-- SPDX-License-Identifier: MIT -->

# `lwip.*` modules

This page describes the modules that wrap the "raw" API of the
[lwIP](https://savannah.nongnu.org/projects/lwip/) library. In general,
functions that can fail return a true value on success, or `(fail, err)` on
failure, where `err` is an `lwip.ERR_*` error code.

lwIP is configured through [`lwipopts.h`](../lib/pico/include_lwip/lwipopts.h).
Many options can be adjusted through compile definitions.

The test modules can be useful as usage examples.

## `lwip`

**Module:** [`lwip`](../lib/pico/lwip.c),
build target: `mlua_mod_lwip`,
tests: [`lwip.test`](../lib/pico/lwip.test.lua)

This module provides common functionality for the other lwIP modules.

- `VERSION: integer`\
  `VERSION_STRING: string`\
  The lwIP as an integer (as `0xMMmmrrcc`, where `MM` is the major verison, `mm`
  is the minor version, `rr` is the revision, and `cc` the candidate) and as a
  string.

- `IPV4: boolean`\
  `IPV6: boolean`\
  Indicate if IPv4 and IPv6 are supported, respectively.

- `ERR_*: integer`\
  Error codes returned by lwIP functions.

- `SOF_*: integer`\
  Socket flags.

- `IPADDR_TYPE_*: integer`\
  IP address types.

- `IP_ANY_TYPE: IPAddr`\
  The "any" IP address.

- `init()`\
  Initialize the IP stack.

- `deinit()`\
  Deinitialize the IP stack.

- `err_str(err) -> string`\
  Convert the error code `err` to a string.

- `assert(ok, err, ...) -> (ok, err, ...)`\
  If `ok` is true, return all arguments unchanged. If `ok` is false, convert the
  error code `err` to a string and raise an error with that string.

- `ipaddr_aton(s) -> IPAddr | (fail, err)`\
  Parse the string representation of an IP address to an `IPAddr` object.

### `IPAddr`

The `IPAddr` type (`lwip.IPAddr`) represents an IP address.

- `IPAddr:type() -> integer`\
  Return the type of the IP address, one of the `lwip.IPADDR_TYPE_*` constants.

- `IPAddr:is_any() -> boolean`\
  `IPAddr:is_muticast() -> boolean`\
  `IPAddr:is_loopback() -> boolean`\
  `IPAddr:is_linklocal() -> boolean`\
  Return `true` iff the address is an "any", multicast, loopback or link-local
  address, respectively.

- `IPAddr:is_broadcast(netif) -> boolean`\
  Return `true` iff the address is a broadcast address on the given
  [NETIF](#netif).

- `IPAddr:__eq(other) -> boolean`\
  Implement the equality operator.

- `IPAddr:__tostring() -> string`\
  Implement the string conversion operator.

## `lwip.dns`

**Module:** [`lwip.dns`](../lib/pico/lwip.dns.c),
build target: `mlua_mod_lwip.dns`,
tests: [`lwip.dns.test`](../lib/pico/lwip.dns.test.lua)

This module provides DNS resolution functionality.

- `ADDRTYPE_*: integer`\
  Address resolution types.

- `MAX_SERVERS: integer`\
  The maximum supported number of DNS servers.

- `getserver(index) -> IPAddr | nil`\
  Return the DNS server at the given zero-based index, or `nil` if the slot
  isn't set.

- `setserver(index, addr)`\
  Set the DNS server at the given zero-based index, or unset it if `addr` is
  `nil`.

- `gethostbyname(host, addrtype = ADDRTYPE_DEFAULT, deadline = nil) -> IPAddr | false | (fail, err)` *[yields]*\
  Resolve a hostname to an IP address. Returns `false` if the hostname cannot be
  found. The deadline is an [absolute time](mlua.md#absolute-time).

## `lwip.ip4`

**Module:** [`lwip.ip4`](../lib/pico/lwip.ip4.c),
build target: `mlua_mod_lwip.ip4`,
tests: [`lwip.ip4.test`](../lib/pico/lwip.ip4.test.lua)

This module provides IPv4-specific functionality.

- `ANY: IPAddr`\
  `LOOPBACK: IPAddr`\
  `BROADCAST: IPAddr`\
  The IPv4 "any", loopback and broadcast addresses, respectively.

- `network(addr, mask) -> IPAddr`\
  Return the address of the network for the given IPv4 address and netmask.

## `lwip.ip6`

**Module:** [`lwip.ip6`](../lib/pico/lwip.ip6.c),
build target: `mlua_mod_lwip.ip6`,
tests: [`lwip.ip6.test`](../lib/pico/lwip.ip6.test.lua)

This module provides IPv6-specific functionality.

- `ANY: IPAddr`\
  `LOOPBACK: IPAddr`\
  The IPv6 "any" and loopback addresses, respectively.

- `ADDR_LIFE_*: integer`\
  Special IPv6 addres lifetime values.

- `ADDR_*: integer`\
  IPv6 address states.

- `MULTICAST_SCOPE_*: integer`\
  Multicast scopes.

- `is_global(addr) -> boolean`\
  `is_sitelocal(addr) -> boolean`\
  `is_uniquelocal(addr) -> boolean`\
  `is_ipv4_mapped_ipv6(addr) -> boolean`\
  `is_ipv4_compat(addr) -> boolean`\
  Return true iff the given address is a general unicast, site-local,
  unique-local, IPv4-mapped or IPv4-compatible address, respectively.

- `multicast_scope(addr) -> integer`\
  Return the scope of the given multicast address.

## `lwip.netif`

**Module:** [`lwip.netif`](../lib/pico/lwip.netif.c),
build target: `mlua_mod_lwip.netif`,
tests: [`lwip.netif.test`](../lib/pico/lwip.netif.test.lua)

This module exposes functionality related to network interfaces.

- `FLAG_*: integer`\
  Network interface flags.

- `IPV6_NUM_ADDRESSES: integer`\
  The maximum number of IPv6 addresses that can be assigned to a `NETIF`.

- `default() -> NETIF | nil`\
  Return the default network interface, or `nil` if there is none.

- `find(key) -> NETIF | (fail, err)`\
  Find a network interface by name or one-based index.

- `iter() -> iterator`\
  Return an iterator over network interfaces.

### `NETIF`

The `NETIF` type (`lwip.NETIF`) represents a network interface.

- `NETIF:index() -> integer`\
  Return the one-based index of the network interface.

- `NETIF:name() -> string`\
  Return the name of the network interface.

- `NETIF:flags() -> integer`\
  Return the flags of the network interface, a bit field of `FLAG_*` values.

- `NETIF:hwaddr([addr]) -> string`\
  Return the hardware address of the network interface, and if `addr` is
  provided, set the hardware address.

- `NETIF:ip4() -> (addr, mask, gw) | (fail, err)`\
  Return the IPv4 address, netmask and gateway assigned to the network
  interface.

- `NETIF:set_ip4(addr = nil, mask = nil, gw = nil) -> true | (fail, err)`\
  Assign the IPv4 address, netmask and / or gateway of the network interface.
  `nil` arguments leave the corresponding attribute unchanged.

- `NETIF:ip6([index]) -> (addr, state, valid, pref) | iterator | (fail, err)`\
  Return the IPv6 address, state (a bit field of `ip6.ADDR_*` values), valid and
  preferred lifetime of the network interface at the given zero-based index. If
  `index` isn't provided, return an iterator over the IPv6 addresses of the
  interface, which returns `(index, addr, state, valid, pref)` values.

- `NETIF:set_ip6(index, addr = nil, state = nil, valid = nil, pref = nil) -> true | (fail, err)`\
  Set the IPv6 address, state, valid and / or preferred lifetime of the network
  interface. `nil` arguments leave the corresponding attribute unchanged.

- `NETIF:add_ip6(index, addr) -> integer | (fail, err)`\
  Add an IPv6 address to the network interface, and set it as "tentative".
  Returns the index of the added address.

- `NETIF:mtu() -> (integer, integer)`\
  Return the MTU and, if IPv6 is enabled, the IPv6 MTU of the network interface.

- `NETIF:up([value]) -> boolean`\
  Return true iff the network interface is up (i.e. the `FLAG_UP` is set). If
  `value` is provided, set the new state.

- `NETIF:link_up([value]) -> boolean`\
  Return true iff the link on the network interface is up (i.e. the
  `FLAG_LINK_UP` is set). If `value` is provided, set the new state.

## `lwip.pbuf`

**Module:** [`lwip.pbuf`](../lib/pico/lwip.pbuf.c),
build target: `mlua_mod_lwip.pbuf`,
tests: [`lwip.pbuf.test`](../lib/pico/lwip.pbuf.test.lua)

This module exposes functionality related to packet buffers (`PBUF`).

- `TRANSPORT: integer`\
  `IP: integer`\
  `LINK: integer`\
  `RAW_TX: integer`\
  `RAW: integer`\
  Packet buffer layers.

- `RAM: integer`\
  `ROM: integer`\
  `REF: integer`\
  `POOL: integer`\
  Packet buffer types.

- `alloc(layer, size, type = RAM) -> PBUF | (fail, err)`\
  Allocate a packet buffer.

### `PBUF`

The `PBUF` type (`lwip.PBUF`) represents a packet buffer, and implements the
[buffer protocol](core.md#buffer-protocol). Its content can be accessed through
the functions provided by the [`mlua.mem`](mlua.md#mluamem) module.

- `PBUF:free()`\
  `PBUF:__close()`\
  `PBUF:__gc()`\
  Deallocate the packet buffer.

- `PBUF:__len() -> integer`\
  Return the size of the packet buffer.

- `PBUF:__buffer() -> (ptr, size, vtable)`\
  Implement the [buffer protocol](core.md#buffer-protocol).

## `lwip.stats`

**Module:** [`lwip.stats`](../lib/pico/lwip.stats.c),
build target: `mlua_mod_lwip.stats`,
tests: [`lwip.stats.test`](../lib/pico/lwip.stats.test.lua)

This module exposes statistics about lwIP. Statistics collection can be enabled
for individual subsystems by setting the compile definitions
`LWIP_{SUBSYS}_STATS=1`. When collection is disabled for a subsystem, the
corresponding accessor function is set to `false`.

- `COUNTER_MAX: integer`\
  The maximum value of a statistics counter. If a counter exceeds this value, it
  wraps around to zero.

- `MEMP_COUNT: integer`\
  The number of memory pools.

- `SUBSYSTEMS: table`\
  The full set of subsystems, regardless of whether statistics collection is
  enabled or disabled for them. The keys are subsystem names, and the values are
  `true`.

- `link() -> table`\
  `etharp() -> table`\
  `ip() -> table`\
  `ip6() -> table`\
  `ip_frag() -> table`\
  `ip6_frag() -> table`\
  `icmp() -> table`\
  `icmp6() -> table`\
  `nd6() -> table`\
  `udp() -> table`\
  `tcp() -> table`\
  Return statistics for a protocol subsystem. The returned table contains keys
  for each of the fields in
  [`struct stats_proto`](https://github.com/lwip-tcpip/lwip/blob/master/src/include/lwip/stats.h).

- `igmp() -> table`\
  `mld6() -> table`\
  Return statistics about IGMP or MLD6. The returned table contains keys for
  each of the fields in
  [`struct stats_igmp`](https://github.com/lwip-tcpip/lwip/blob/master/src/include/lwip/stats.h).

- `mem() -> table`\
  Return statistics about memory allocation on the heap. The returned table
  contains keys for each of the fields in
  [`struct stats_mem`](https://github.com/lwip-tcpip/lwip/blob/master/src/include/lwip/stats.h).

- `mem_reset()`\
  Reset the fields in heap memory allocation statistics that track a maximum.

- `memp(index) -> (table, string | nil)`\
  Return statistics about memory allocation in the memory pool with the given
  zero-based index. The returned table contains keys for each of the fields in
  [`struct stats_mem`](https://github.com/lwip-tcpip/lwip/blob/master/src/include/lwip/stats.h).

- `memp_reset(index)`\
  Reset the fields in memory pool allocation statistics that track a maximum.

- `mib2() -> table`\
  Returns SNMP MIB2 statistics. The returned table contains keys for each of the
  fields in
  [`struct stats_mib2`](https://github.com/lwip-tcpip/lwip/blob/master/src/include/lwip/stats.h).

## `lwip.tcp`

**Module:** [`lwip.tcp`](../lib/pico/lwip.tcp.c),
build target: `mlua_mod_lwip.tcp`,
tests: [`lwip.tcp.test`](../lib/pico/lwip.tcp.test.lua)

This module exposes TCP functionality.

- `DEFAULT_LISTEN_BACKLOG: integer`\
  The default backlog value for `TCP:listen()`.

- `WRITE_FLAG_*: integer`\
  Write flags.

- `PRIO_*: integer`\
  Priority values.

- `new(type = lwip.IPADDR_TYPE_ANY) -> TCP | (fail, err)`\
  Create a new TCP socket.

### `TCP`

The `TCP` type (`lwip.TCP`) represents a TCP socket.

- `TCP:close() -> true | (fail, err)`\
  `TCP:__close()`\
  `TCP:__gc()`\
  Close the socket.

- `TCP:shutdown(rx = false, tx = false) -> true | (fail, err)`\
  Shut down either or both directions of a connection.

- `TCP:bind(addr = lwip.IP_ANY_TYPE, port) -> true | (fail, err)`\
  Bind the socket to a local IP address and port.

- `TCP:bind(netif = nil) -> true | (fail, err)`\
  Bind the socket to a [NETIF](#netif), or unbind it if `netif` is `nil`.

- `TCP:listen(backlog = DEFAULT_LISTEN_BACKLOG) -> true | (fail, err)`\
  Set up the socket to enable accepting connections.

- `TCP:accept(deadline = nil) -> TCP | (fail, err)` *[yields]*\
  Accept an incoming connection. Blocks if there are no incoming connections in
  the backlog. The deadline is an [absolute time](mlua.md#absolute-time).

- `TCP:connect(addr, port, deadline = nil) -> true | (fail, err)` *[yields]*\
  Initiate an outgoing connection. Blocks until the connection is established.
  The deadline is an [absolute time](mlua.md#absolute-time).

- `TCP:send(data, ..., deadline = nil) -> true | (fail, err)` *[yields]*\
  `TCP:write(data, ..., deadline = nil) -> true | (fail, err)` *[yields]*\
  Write data to the connection. The deadline is an
  [absolute time](mlua.md#absolute-time).

- `TCP:recv(len, deadline = nil) -> string | (fail, err)` *[yields]*\
  `TCP:read(len, deadline = nil) -> string | (fail, err)` *[yields]*\
  Read data from the connection. The deadline is an
  [absolute time](mlua.md#absolute-time).

- `TCP:prio([value]) -> integer`\
  Return the priority of the socket. If `value` is provided, set a new
  priority.

- `TCP:local_ip() -> IPAddr`\
  `TCP:remote_ip() -> IPAddr`\
  Return the local and remote IP address of the socket, respectively.

- `TCP:local_port() -> integer`\
  `TCP:remote_port() -> integer | (fail, err)`\
  Return the local and remote port of the socket, respectively.

- `TCP:options(set = nil, clear = nil) -> integer`\
  Return the options set on the socket, a bit field of `lwip.SOF_*` values. If
  `set` and / or `clear` are provided, set or clear the corresponding options.

- `TCP:tos([value]) -> integer`\
  Return the TOS of the socket. If `value` is provided, set a new TOS.

- `TCP:ttl([value]) -> integer`\
  Return the TTL of the socket. If `value` is provided, set a new TTL.

## `lwip.udp`

**Module:** [`lwip.udp`](../lib/pico/lwip.udp.c),
build target: `mlua_mod_lwip.udp`,
tests: [`lwip.udp.test`](../lib/pico/lwip.udp.test.lua)

This module exposes UDP functionality.

- `new(type = lwip.IPADDR_TYPE_ANY, rx_queue = 4) -> UDP | (fail, err)`\
  Create a new UDP socket. The size of the receive queue can be adjusted;
  packets received while the queue is full are silently dropped. The size can be
  set to zero for send-only sockets.

### `UDP`

The `UDP` type (`lwip.UDP`) represents a UDP socket.

- `UDP:close() -> true | (fail, err)`\
  `UDP:__close()`\
  `UDP:__gc()`\
  Close the socket.

- `UDP:bind(addr = lwip.IP_ANY_TYPE, port) -> true | (fail, err)`\
  Bind the socket to a local IP address and port.

- `UDP:bind_netif(netif = nil) -> true | (fail, err)`\
  Bind the socket to a [NETIF](#netif), or unbind it if `netif` is `nil`.

- `UDP:connect(addr = lwip.IP_ANY_TYPE, port) -> true | (fail, err)`\
  Set the remote end of the socket.

- `UDP:disconnect()`\
  Remove the remote end of the socket.

- `UDP:send(pbuf) -> true | (fail, err)`\
  Send a `PBUF` to the remote IP and port set on the socket.

- `UDP:sendto(pbuf, addr, port, netif = nil, src = nil) -> true | (fail, err)`\
  Send a `PBUF` to the remote IP and port set on the socket. Optionally, the
  `NETIF` on which the packet should be sent, and the source address of the
  packet, can be specified (`netif` is required if `src` is specified).

- `UDP:recv(deadline = nil) -> PBUF | (fail, err)` *[yields]*\
  Read a packet from the receive queue. Blocks if the receive queue is empty.
  The deadline is an [absolute time](mlua.md#absolute-time).

- `UDP:recvfrom(deadline = nil) -> (PBUF, IPAddr, integer) | (fail, err)` *[yields]*\
  Read a packet from the receive queue. Returns the packet data and the source
  address and port. Blocks if the receive queue is empty. The deadline is an
  [absolute time](mlua.md#absolute-time).

- `UDP:local_ip() -> IPAddr`\
  `UDP:remote_ip() -> IPAddr`\
  Return the local and remote IP address of the socket, respectively.

- `UDP:local_port() -> integer`\
  `UDP:remote_port() -> integer`\
  Return the local and remote port of the socket, respectively.

- `UDP:options(set = nil, clear = nil) -> integer`\
  Return the options set on the socket, a bit field of `lwip.SOF_*` values. If
  `set` and / or `clear` are provided, set or clear the corresponding options.

- `UDP:tos([value]) -> integer`\
  Return the TOS of the socket. If `value` is provided, set a new TOS.

- `UDP:ttl([value]) -> integer`\
  Return the TTL of the socket. If `value` is provided, set a new TTL.
