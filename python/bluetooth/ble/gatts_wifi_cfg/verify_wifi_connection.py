#!/usr/bin/env python3
"""Verify Wi-Fi station association and gateway reachability."""

from __future__ import annotations

import argparse
import logging
import subprocess
import sys
import time

import wifi_controller

DEFAULT_SSID = "WiFi_Controller"
DEFAULT_PASSWORD = "12345678"
DEFAULT_GATEWAY = "192.168.5.1"
DEFAULT_INTERFACE = "wlan0"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ssid", default=DEFAULT_SSID)
    parser.add_argument("--password", default=DEFAULT_PASSWORD)
    parser.add_argument("--gateway", default=DEFAULT_GATEWAY)
    parser.add_argument("--iface", default=DEFAULT_INTERFACE)
    parser.add_argument("--timeout", type=int, default=30)
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--no-connect", action="store_true")
    parser.add_argument("--verbose", "-v", action="store_true")
    return parser.parse_args()


def report(interface: str, gateway: str, log: logging.Logger) -> wifi_controller.WifiConnection:
    connection = wifi_controller.current_connection(interface)
    reachable = wifi_controller.ping(gateway)
    log.info("state: ssid=%s ip=%s gateway=%s signal=%s ping=%s", connection.ssid, connection.ip, connection.gateway, connection.signal, reachable)
    return connection


def main() -> int:
    args = parse_args()
    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")
    log = logging.getLogger("verify_wifi_connection")
    mac = wifi_controller.interface_mac(args.iface)
    log.info("interface=%s mac=%s", args.iface, mac or "unknown")
    if not args.no_connect:
        try:
            result = wifi_controller.connect(args.ssid, args.password, force=args.force, interface=args.iface, timeout=args.timeout)
            log.info("connected: ssid=%s ip=%s backend=%s", result.ssid, result.ip, result.backend)
        except (OSError, TimeoutError, subprocess.CalledProcessError) as error:
            log.error("Wi-Fi connection failed: %s", error)
            return 2
    deadline = time.monotonic() + args.timeout
    while time.monotonic() < deadline:
        connection = report(args.iface, args.gateway, log)
        if connection.ssid == args.ssid and connection.ip and wifi_controller.ping(args.gateway, count=1):
            print(f"Wi-Fi 验证通过：SSID={connection.ssid} IP={connection.ip} MAC={mac or 'unknown'}")
            return 0
        time.sleep(1)
    print(f"Wi-Fi 验证失败：无法通过网关 {args.gateway}")
    return 2


if __name__ == "__main__":
    sys.exit(main())
