#!/usr/bin/env python
# Copyright 2024 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

import contextlib
import re
import socket
import socketserver
import sys
import threading
import time

CONTROL_PORT = 8000
PORTS = list(range(8001, 8010))


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
def udp_socket(port):
    sock = socket.socket(family=socket.AF_INET6, type=socket.SOCK_DGRAM)
    try:
        sock.settimeout(1)
        sock.bind(('', port))
        yield sock
    finally:
        sock.close()


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
    def __init__(self, addr, stdout):
        super().__init__(addr, ControlHandler)
        self.out = stdout
        self.lock = threading.Lock()
        self.udp_ports = set(PORTS)

    @contextlib.contextmanager
    def udp_port(self):
        try:
            with self.lock:
                port = self.udp_ports.pop()
        except KeyError:
            raise RuntimeError("No UDP port available")
        try:
            yield port
        finally:
            with self.lock:
                self.udp_ports.add(port)


class ControlHandler(socketserver.StreamRequestHandler):
    disable_nagle_algorithm = True
    timeout = 1

    def handle(self):
        out = self.server.out
        out.write(f"Control connection from {self.client_address}\n")
        try:
            self.w = SyncWriter(self.wfile)

            # Detect hard disconnections quickly.
            self.request.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            self.request.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, 1)
            self.request.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, 1)
            self.request.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, 5)

            with self.server.udp_port() as lport:
                self.w.write(f'PORT {lport}\n')

                # Handle command.
                line = self.rfile.readline().decode('utf-8')
                if not line: return
                line = line.removesuffix('\n').removesuffix('\r')
                out.write(f"Command: {line}\n")
                cmd, *args = line.split(' ')
                try:
                    getattr(self, f'cmd_{cmd}')(lport, *args)
                    self.w.write('DONE\n')
                except Exception:
                    self.w.write('ERROR\n')
                    raise
        finally:
            out.write(f"Control disconnection from {self.client_address}\n")

    def cmd_udp_send(self, lport, rport, size, count, interval):
        rport, size = int(rport), int(size)
        count, interval = int(count), int(interval)
        dest = (self.client_address[0], rport)
        with udp_socket(lport) as sock:
            for pid in range(count):
                data = generate_data(size, pid)
                sock.sendto(data, dest)
                time.sleep(interval * 1e-6)

    def cmd_udp_recv(self, lport, rport):
        rport = int(rport)
        src = (self.client_address[0], rport)
        with udp_socket(lport) as sock:
            while True:
                data, addr = sock.recvfrom(1 << 20)
                if addr == src:
                    size = len(data)
                    pid, ok = verify_data(data)
                    res = 'OK' if ok else 'BAD'
                    self.w.write(f'RECV {size} {pid} {res}\n')

    def cmd_udp_echo(self, lport, rport):
        rport = int(rport)
        src = (self.client_address[0], rport)
        with udp_socket(lport) as sock:
            while True:
                data, addr = sock.recvfrom(1 << 20)
                self.server.out.write(f"Received UDP packet from {addr}\n")
                if addr == src:
                    size = len(data)
                    pid, ok = verify_data(data)
                    res = 'OK' if ok else 'BAD'
                    self.w.write(f'RECV {size} {pid} {res}\n')
                    sock.sendto(data, addr)


def main(argv, stdin, stdout, stderr):
    with ControlServer(('', CONTROL_PORT), stdout) as server:
        server.serve_forever()


if __name__ == '__main__':
    sys.exit(main(sys.argv, sys.stdin, sys.stdout, sys.stderr))
