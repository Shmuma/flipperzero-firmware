import logging
import argparse
import subprocess
import typing as tt
from flipper.utils.cdc import resolve_port
import os
import sys

import time


def get_command(port: str) -> tt.List[str]:
    return [
        os.path.basename(sys.executable),
        "-m",
        "serial.tools.miniterm",
        "--raw",
        port,
        "230400",
    ]


def feed_commands(port: str, commands: tt.List[str]):
    """
    Execute miniterm and feed given commands one by one. Then terminate the process.
    :param port: port to connect
    :param commands: list of commands to execute
    """
    proc = subprocess.Popen(get_command(port), stdin=subprocess.PIPE, close_fds=True)
    try:
        time.sleep(1)
        r = proc.stdin.write(b'help\n')
        print(r)
        time.sleep(1)
        # r = proc.stderr.read()
        # print(r)
        # res = proc.communicate(b'help\n')
        # print(res)
        # time.sleep(10)
        # for cmd in commands:
        #     cmd_b = (cmd + "\n").encode()
        #     proc.stdin.write(cmd_b)
        #     time.sleep(1)
        #     res = proc.stderr.read()
        #     print(res)
        #     time.sleep(10)
    finally:
        proc.terminate()
        proc.wait()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-e", "--exec", action='append', help="Execute one or more commands and exit. "
                                                              "Option could be given several times.")
    args = parser.parse_args()
    logger = logging.getLogger()
    if not (port := resolve_port(logger, "auto")):
        logger.error("Is Flipper connected over USB and isn't in DFU mode?")
        return 1
    if not args.exec:
        subprocess.call(get_command(port))
    else:
        feed_commands(port, args.exec)


if __name__ == "__main__":
    main()
