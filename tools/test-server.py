#!/usr/bin/env python
# Copyright 2024 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

import argparse
import contextlib
import os.path
import re
import socket
import socketserver
import sys
import threading
import time


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
        sock.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)
        sock.settimeout(1)
        sock.bind(('::', port))
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
    timeout = 1

    def handle(self):
        with self.server.port() as lport:
            out = self.server.out
            out.write(f"[{lport}] Connection: {self.client_address}\n")
            self.w = SyncWriter(self.wfile)

            # Detect hard disconnections quickly.
            self.request.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            self.request.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, 1)
            self.request.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, 1)
            self.request.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, 5)

            # Report the allocated test port.
            self.w.write(f'PORT {lport}\n')

            # Read and decode the command.
            line = self.rfile.readline().decode('utf-8')
            if not line: return
            line = line.removesuffix('\n').removesuffix('\r')
            out.write(f"[{lport}] {line}\n")
            cmd, *args = line.split(' ')

            # Run the command.
            try:
                getattr(self, f'cmd_{cmd}')(lport, *args)
                self.w.write('DONE\n')
            except Exception:
                self.w.write('ERROR\n')
                raise

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
                if addr[:2] == src:
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
                if addr[:2] == src:
                    size = len(data)
                    pid, ok = verify_data(data)
                    res = 'OK' if ok else 'BAD'
                    self.w.write(f'RECV {size} {pid} {res}\n')
                    sock.sendto(data, addr)


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
