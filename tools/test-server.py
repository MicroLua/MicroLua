#!/usr/bin/env python
# Copyright 2024 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

import argparse
from concurrent import futures
import contextlib
import os.path
import re
import socket
import socketserver
import sys
import threading
import time
import traceback


def log_exception(fn):
    def wrap(*args, **kwargs):
        try:
            return fn(*args, **kwargs)
        except Exception:
            traceback.print_exc()
            raise
    return wrap


def prng(seed):
    seed = (1103515245 * seed + 12345) & 0x7fffffff
    return seed, (seed >> 23) & 0xff


def generate_data(size, pid):
    seed = size + pid
    def gen():
        nonlocal seed
        yield pid & 0xff
        yield (pid >> 8) & 0xff
        for _ in range(size - 2):
            seed, v = prng(seed)
            yield v
    return bytearray(gen())


def verify_data(data):
    size = len(data)
    pid = data[0] | (data[1] << 8)
    seed = size + pid
    for off in range(2, size):
        d = data[off]
        seed, v = prng(seed)
        if d != v: return pid, False
    return pid, True


@contextlib.contextmanager
def tcp_connect(addr):
    sock = socket.socket(family=socket.AF_INET6, type=socket.SOCK_STREAM)
    try:
        sock.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)
        sock.settimeout(1)
        sock.connect(addr)
        yield sock
    finally:
        sock.close()


@contextlib.contextmanager
def tcp_listen(port):
    sock = socket.socket(family=socket.AF_INET6, type=socket.SOCK_STREAM)
    try:
        sock.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.settimeout(0.2)
        sock.bind(('::', port))
        sock.listen()
        yield sock
    finally:
        sock.close()


@contextlib.contextmanager
def tcp_accept(sock):
    conn, addr = sock.accept()
    try:
        conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, True)
        sock.settimeout(0.2)
        yield conn, addr
    finally:
        conn.close()


@contextlib.contextmanager
def udp_socket(port):
    sock = socket.socket(family=socket.AF_INET6, type=socket.SOCK_DGRAM)
    try:
        sock.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)
        sock.settimeout(0.2)
        sock.bind(('::', port))
        yield sock
    finally:
        sock.close()


def recv_all(sock, size):
    data = bytearray(size)
    view = memoryview(data)
    while view:
        cnt = sock.recv_into(view, len(view))
        if cnt == 0: return data[:-len(view)]
        view = view[cnt:]
    return data


class SyncWriter:
    def __init__(self, w):
        self.w, self.lock = w, threading.Lock()

    def write(self, data):
        if isinstance(data, str):
            data = data.encode('utf-8')
        with self.lock:
            return self.w.write(data)


class TCPServer(socketserver.ThreadingTCPServer):
    address_family = socket.AF_INET6
    allow_reuse_address = True


class ControlServer(TCPServer):
    def __init__(self, config, stdout):
        super().__init__(('', config.control_port), ControlHandler)
        self.out = stdout
        self.lock = threading.Lock()
        self.ports = set(config.test_ports)

    @contextlib.contextmanager
    def port(self):
        try:
            with self.lock:
                port = self.ports.pop()
        except KeyError:
            raise RuntimeError("No port available")
        try:
            yield port
        finally:
            with self.lock:
                self.ports.add(port)


class ControlHandler(socketserver.StreamRequestHandler):
    disable_nagle_algorithm = True
    timeout = 0.2

    def handle(self):
        with self.server.port() as lport:
            self.server.out.write(
                f"[{lport}] Connection: {self.client_address}\n")
            try:
                self.handle_conn(lport)
            except TimeoutError:
                self.server.out.write(f"[{lport}] Timeout\n")
            except BrokenPipeError:
                self.server.out.write(f"[{lport}] Broken pipe\n")
            except Exception as e:
                self.server.out.write(f"[{lport}] Exception:\n")
                traceback.print_exc(file=self.server.out)
            else:
                self.server.out.write(f"[{lport}] Done\n")

    def handle_conn(self, lport):
        # Detect hard disconnections quickly.
        conn = self.request
        conn.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
        conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, 1)
        conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, 1)
        conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, 5)

        # Report the allocated test port.
        self.w = SyncWriter(self.wfile)
        self.w.write(f'PORT {lport}\n')

        # Read and decode the command.
        line = self.rfile.readline().decode('utf-8')
        if not line: return
        line = line.removesuffix('\n').removesuffix('\r')
        self.server.out.write(f"[{lport}] {line}\n")
        cmd, *args = line.split(' ')

        # Run the command.
        getattr(self, f'cmd_{cmd}')(lport, *args)

    def cmd_tcp_connect(self, lport, rport, conns, size, count, interval):
        rport, conns, size = int(rport), int(conns), int(size)
        count, interval = int(count), int(interval)
        dest = (self.client_address[0], rport) + self.client_address[2:]
        with futures.ThreadPoolExecutor(max_workers=conns) as pool:
            for i in range(conns):
                pool.submit(self.handle_connect, dest, i * count, size, count,
                            interval)

    def handle_connect(self, dest, pid_base, size, count, interval):
        with tcp_connect(dest) as conn:
            with futures.ThreadPoolExecutor(max_workers=2) as pool:
                pool.submit(self.handle_tcp_send, conn, pid_base, size, count,
                            interval)
                pool.submit(self.handle_tcp_recv, conn)

    def cmd_tcp_listen(self, lport, rport, size, count, interval):
        rport, size = int(rport), int(size)
        count, interval = int(count), int(interval)
        src = (self.client_address[0], rport)
        with tcp_listen(lport) as sock:
            self.w.write('LISTENING\n')
            with tcp_accept(sock) as (conn, addr):
                if addr[:2] != src:
                    raise RuntimeError(f"Wrong peer: {addr}")
                with futures.ThreadPoolExecutor(max_workers=2) as pool:
                    pool.submit(self.handle_tcp_send, conn, 0, size, count,
                                interval)
                    pool.submit(self.handle_tcp_recv, conn)

    def handle_tcp_send(self, conn, pid_base, size, count, interval):
        try:
            dsize = bytearray([size & 0xff, (size >> 8) & 0xff])
            for pid in range(pid_base, pid_base + count):
                data = generate_data(size, pid)
                conn.sendall(dsize + data)
                time.sleep(interval * 1e-6)
        finally:
            conn.shutdown(socket.SHUT_WR)

    def handle_tcp_recv(self, conn):
        try:
            while True:
                dsize = recv_all(conn, 2)
                if len(dsize) < 2: break
                size = dsize[0] | (dsize[1] << 8)
                data = recv_all(conn, size)
                if len(data) < size: break
                pid, ok = verify_data(data)
                res = 'OK' if ok else 'BAD'
                self.w.write(f'RECV {size} {pid} {res}\n')
        finally:
            conn.shutdown(socket.SHUT_RD)

    def cmd_udp(self, lport, rport, size, count, interval):
        rport, size = int(rport), int(size)
        count, interval = int(count), int(interval)
        with udp_socket(lport) as sock:
            self.w.write('LISTENING\n')
            with futures.ThreadPoolExecutor(max_workers=2) as pool:
                pool.submit(self.handle_udp_send, sock, rport, size, count,
                            interval)
                pool.submit(self.handle_udp_recv, sock, rport)

    def handle_udp_send(self, sock, rport, size, count, interval):
        dest = (self.client_address[0], rport) + self.client_address[2:]
        for pid in range(count):
            data = generate_data(size, pid)
            sock.sendto(data, dest)
            time.sleep(interval * 1e-6)

    def handle_udp_recv(self, sock, rport):
        src = (self.client_address[0], rport)
        while True:
            data, addr = sock.recvfrom(1 << 20)
            if addr[:2] == src:
                size = len(data)
                pid, ok = verify_data(data)
                res = 'OK' if ok else 'BAD'
                self.w.write(f'RECV {size} {pid} {res}\n')


range_re = re.compile('^([0-9]+)-([0-9]+)$')


def parse_ports(s):
    ports = set()
    for part in s.split(','):
        if m := range_re.match(part):
            ports.update(range(int(m.group(1)), int(m.group(2)) + 1))
        else:
            ports.add(int(part))
    return ports


def main(argv, stdin, stdout, stderr):
    class Parser(argparse.ArgumentParser):
        def _print_message(self, message, file=None):
            super()._print_message(message, stderr)
    parser = Parser(
        prog=os.path.basename(argv[0]), add_help=False,
        usage='%(prog)s [options]',
        description="Run a test server for networked unit tests.")
    arg = parser.add_argument_group("Options").add_argument
    arg('--help', action='help', help="Show this help message and exit.")
    arg('--control-port', metavar='PORT', type=int, dest='control_port',
        default=8000, help="The control port number (default: %(default)s).")
    arg('--test-ports', metavar='PORTS', type=parse_ports, dest='test_ports',
        default='8001-8009',
        help="A comma-separated list of ports and port ranges "
             "(default: %(default)s).")

    config = parser.parse_args(argv[1:])
    with ControlServer(config, stdout) as server:
        server.serve_forever()


if __name__ == '__main__':
    sys.exit(main(sys.argv, sys.stdin, sys.stdout, sys.stderr))
