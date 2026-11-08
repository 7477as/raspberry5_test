#!/usr/bin/env python3
"""NetworkManager and wpa_cli based Wi-Fi station controller."""

from __future__ import annotations

import dataclasses
import logging
import re
import shutil
import subprocess
import time
from typing import Sequence

LOGGER = logging.getLogger(__name__)
DEFAULT_INTERFACE = "wlan0"
DEFAULT_TIMEOUT = 30


@dataclasses.dataclass(frozen=True)
class WifiConnection:
    ssid: str = ""
    ip: str = ""
    gateway: str = ""
    signal: int = 0


@dataclasses.dataclass(frozen=True)
class WifiConnectResult:
    ssid: str
    ip: str
    gateway: str
    signal: int
    backend: str


def run_command(arguments: Sequence[str], timeout: int = 15, check: bool = True) -> subprocess.CompletedProcess[str]:
    try:
        return subprocess.run(arguments, check=check, capture_output=True, text=True, timeout=timeout)
    except subprocess.CalledProcessError as error:
        LOGGER.error("command failed: %s; stderr=%s", " ".join(arguments), (error.stderr or "").strip())
        raise


def network_manager_running() -> bool:
    if shutil.which("nmcli") is None:
        return False
    try:
        result = run_command(["nmcli", "-t", "-f", "RUNNING", "general"], timeout=5)
        return result.stdout.strip().lower() == "running"
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
        return False


def current_connection(interface: str = DEFAULT_INTERFACE) -> WifiConnection:
    if network_manager_running():
        try:
            result = run_command(
                ["nmcli", "-t", "-f", "GENERAL.STATE,GENERAL.CONNECTION,IP4.ADDRESS,IP4.GATEWAY", "device", "show", interface],
                timeout=5,
                check=False,
            )
            return parse_network_manager_state(result.stdout)
        except subprocess.TimeoutExpired:
            return WifiConnection()
    if shutil.which("wpa_cli") is None:
        return WifiConnection()
    try:
        result = run_command(["wpa_cli", "-i", interface, "status"], timeout=5, check=False)
        return parse_wpa_state(result.stdout)
    except subprocess.TimeoutExpired:
        return WifiConnection()


def parse_network_manager_state(text: str) -> WifiConnection:
    connection = WifiConnection()
    values: dict[str, str] = {}
    for line in text.splitlines():
        key, separator, value = line.partition(":")
        if separator:
            values[key.rsplit(".", 1)[-1].upper()] = value.strip()
    ip_address = values.get("ADDRESS", "").split("/", 1)[0]
    return dataclasses.replace(
        connection,
        ssid=values.get("CONNECTION", ""),
        ip=ip_address,
        gateway=values.get("GATEWAY", ""),
    )


def parse_wpa_state(text: str) -> WifiConnection:
    values = dict(line.split("=", 1) for line in text.splitlines() if "=" in line)
    return WifiConnection(ssid=values.get("ssid", ""), ip=values.get("ip_address", ""))


def connect(ssid: str, password: str, force: bool = False, interface: str = DEFAULT_INTERFACE, timeout: int = DEFAULT_TIMEOUT) -> WifiConnectResult:
    if not ssid:
        raise ValueError("ssid cannot be empty")
    if network_manager_running():
        return connect_with_network_manager(ssid, password, force, interface, timeout)
    return connect_with_wpa_cli(ssid, password, force, interface, timeout)


def connect_with_network_manager(ssid: str, password: str, force: bool, interface: str, timeout: int) -> WifiConnectResult:
    profile = f"wifi_config_{ssid}"
    if force:
        run_command(["nmcli", "device", "disconnect", interface], timeout=10, check=False)
    run_command(["nmcli", "connection", "delete", "id", profile], timeout=10, check=False)
    run_command(["nmcli", "connection", "add", "type", "wifi", "ifname", interface, "con-name", profile, "ssid", ssid], timeout=10)
    run_command(["nmcli", "connection", "modify", profile, "wifi-sec.key-mgmt", "wpa-psk", "wifi-sec.psk", password], timeout=10)
    run_command(["nmcli", "connection", "up", profile, "ifname", interface], timeout=timeout)
    return wait_for_connection(ssid, interface, timeout, "nmcli")


def connect_with_wpa_cli(ssid: str, password: str, force: bool, interface: str, timeout: int) -> WifiConnectResult:
    if force:
        run_command(["wpa_cli", "-i", interface, "disconnect"], timeout=5, check=False)
    added = run_command(["wpa_cli", "-i", interface, "add_network"], timeout=5)
    network_id = added.stdout.strip().splitlines()[-1]
    run_command(["wpa_cli", "-i", interface, "set_network", network_id, "ssid", f'"{ssid}"'], timeout=5)
    run_command(["wpa_cli", "-i", interface, "set_network", network_id, "psk", f'"{password}"'], timeout=5)
    run_command(["wpa_cli", "-i", interface, "enable_network", network_id], timeout=5)
    run_command(["wpa_cli", "-i", interface, "save_config"], timeout=5, check=False)
    if shutil.which("dhclient"):
        run_command(["dhclient", interface], timeout=20, check=False)
    return wait_for_connection(ssid, interface, timeout, "wpa_cli")


def wait_for_connection(ssid: str, interface: str, timeout: int, backend: str) -> WifiConnectResult:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        connection = current_connection(interface)
        if connection.ssid == ssid and connection.ip:
            return WifiConnectResult(connection.ssid, connection.ip, connection.gateway, connection.signal, backend)
        time.sleep(1)
    raise TimeoutError(f"timed out waiting for Wi-Fi connection to {ssid}")


def disconnect(interface: str = DEFAULT_INTERFACE) -> None:
    if network_manager_running():
        run_command(["nmcli", "device", "disconnect", interface], timeout=10, check=False)
    elif shutil.which("wpa_cli"):
        run_command(["wpa_cli", "-i", interface, "disconnect"], timeout=5, check=False)


def interface_mac(interface: str = DEFAULT_INTERFACE) -> str:
    try:
        result = run_command(["ip", "link", "show", interface], timeout=5, check=False)
    except subprocess.TimeoutExpired:
        return ""
    match = re.search(r"link/ether\s+([0-9a-fA-F:]{17})", result.stdout)
    return match.group(1).lower() if match else ""


def ping(host: str, count: int = 1, timeout: int = 2) -> bool:
    try:
        result = subprocess.run(["ping", "-c", str(count), "-W", str(timeout), host], check=False, capture_output=True, text=True, timeout=count * timeout + 5)
        return result.returncode == 0
    except (FileNotFoundError, subprocess.TimeoutExpired):
        return False
