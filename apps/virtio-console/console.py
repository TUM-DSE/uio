#!/usr/bin/env python3

import socket
import sys
import os
import time
import fcntl


def main(port="/tmp/port0", txt="hello\n"):
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        fcntl.fcntl(3, fcntl.F_SETFD, fcntl.FD_CLOEXEC)
        s.connect(port)
        s.settimeout(0.1)
        s.send(bytes(txt, 'utf-8'))
        data = b""
        try:
            while True:
                r = s.recv(1024)
                data += r
        except socket.timeout:
            pass

        print(data.decode('utf-8'))


if __name__ == "__main__":
    import fire
    fire.Fire(main)
