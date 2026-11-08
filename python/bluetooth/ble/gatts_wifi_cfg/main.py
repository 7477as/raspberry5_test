#!/usr/bin/env python3
"""BLE GATT Wi-Fi configuration service."""

from __future__ import annotations

import argparse
import logging
import subprocess
import sys
import threading
import time
from datetime import datetime
from typing import Any, Optional

import protocol
import wifi_controller

DEFAULT_LOCAL_NAME = "RPi5_WiFi_Config"
AP_SSID = "RPi5_WiFi"
AP_PASSWORD = "12345678"
AP_BSSID = "AA:BB:CC:00:11:22"
AP_CHANNEL = 6
AP_DEVICE_IP = "192.168.4.1"
AP_CLIENT_IP = "192.168.4.100"
AP_NETMASK = "255.255.255.0"
AP_GATEWAY = "192.168.4.1"
AP_WEBSOCKET_URL = "ws://192.168.4.1:8090/ws"
AP_HTTP_URL = "http://192.168.4.1:8090"
AP_UDP_PORT = 50062


class WifiConfigurationService:
    """Maintain one BLE configuration session and create protocol responses."""

    def __init__(self, log: logging.Logger) -> None:
        self.log = log
        self.session_id = ""
        self.handshake_done = False
        self.sequence = 0

    def reset_session(self) -> None:
        self.session_id = ""
        self.handshake_done = False

    def handle_frame(self, frame: bytes) -> Optional[bytes]:
        try:
            message = protocol.parse_envelope(frame)
        except ValueError as error:
            self.log.warning("invalid protocol frame: %s", error)
            return None
        if message["kind"] != protocol.KIND_REQUEST:
            return None
        request_id = str(message["request_id"])
        command_id = int(message["command_id"])
        payload = bytes(message["payload"])
        if command_id != protocol.COMMAND_DEVICE_HANDSHAKE and not self.handshake_done:
            return protocol.build_response(request_id, protocol.ERROR_HANDSHAKE_REQUIRED)
        if command_id == protocol.COMMAND_DEVICE_HANDSHAKE:
            return self.handle_handshake(request_id)
        if command_id == protocol.COMMAND_PROTOCOL_NEGOTIATE:
            return protocol.build_response(request_id, payload=build_negotiate_response())
        if command_id == protocol.COMMAND_SET_WIFI_MODE:
            return self.handle_wifi_mode(request_id, payload)
        if command_id == protocol.COMMAND_GET_WIFI_MODE:
            return protocol.build_response(request_id, payload=build_wifi_mode_response(protocol.WIFI_MODE_SOFTAP))
        if command_id == protocol.COMMAND_KEEPALIVE:
            return protocol.build_response(request_id)
        return protocol.build_response(request_id, 9)

    def handle_handshake(self, request_id: str) -> bytes:
        self.sequence += 1
        self.session_id = f"CFG{int(time.time() * 1000) % 100000:05d}{self.sequence:04d}"
        self.handshake_done = True
        payload = protocol.encode_varint_field(1, protocol.ROLE_PHONE)
        payload += protocol.encode_varint_field(2, protocol.ERROR_OK)
        payload += protocol.encode_string_field(4, self.session_id)
        self.log.info("handshake accepted: session=%s", self.session_id)
        return protocol.build_response(request_id, payload=payload)

    def handle_wifi_mode(self, request_id: str, payload: bytes) -> bytes:
        request = protocol.parse_wifi_mode_request(payload)
        self.log.info("wifi mode request: mode=%s role=%s ssid=%s", request.target_mode, request.client_role, request.ssid or "<empty>")
        if request.target_mode == protocol.WIFI_MODE_STA and request.ssid:
            threading.Thread(target=self.connect_sta, args=(request.ssid, request.password, request.force), daemon=True).start()
        return protocol.build_response(request_id, payload=build_wifi_mode_response(request.target_mode))

    def connect_sta(self, ssid: str, password: str, force: bool) -> None:
        try:
            result = wifi_controller.connect(ssid, password, force=force)
            self.log.info("Wi-Fi connected: ssid=%s ip=%s gateway=%s backend=%s", result.ssid, result.ip, result.gateway, result.backend)
        except (OSError, TimeoutError, subprocess.CalledProcessError) as error:
            self.log.error("Wi-Fi connection failed: %s", error)


def build_negotiate_response() -> bytes:
    payload = protocol.encode_string_field(1, "2.0")
    payload += protocol.encode_string_field(2, "sta")
    payload += protocol.encode_string_field(2, "softap")
    payload += protocol.encode_string_field(2, "sta_softap")
    payload += protocol.encode_varint_field(3, 512)
    payload += protocol.encode_varint_field(4, 1)
    return payload


def build_wifi_mode_response(mode: int) -> bytes:
    description = "STA" if mode == protocol.WIFI_MODE_STA else "SoftAP"
    payload = protocol.encode_varint_field(1, mode)
    payload += protocol.encode_string_field(2, description)
    connection = protocol.encode_string_field(1, AP_SSID)
    connection += protocol.encode_string_field(2, AP_PASSWORD)
    connection += protocol.encode_string_field(3, AP_BSSID)
    connection += protocol.encode_varint_field(4, AP_CHANNEL)
    connection += protocol.encode_varint_field(5, 1)
    connection += protocol.encode_string_field(6, AP_CLIENT_IP)
    connection += protocol.encode_string_field(7, AP_DEVICE_IP)
    connection += protocol.encode_string_field(8, AP_NETMASK)
    connection += protocol.encode_string_field(9, AP_GATEWAY)
    payload += protocol.encode_bytes_field(3, connection)
    payload += protocol.encode_string_field(4, AP_WEBSOCKET_URL)
    payload += protocol.encode_string_field(5, AP_HTTP_URL)
    payload += protocol.encode_varint_field(6, AP_UDP_PORT)
    return payload


def create_callbacks(service: WifiConfigurationService, log: logging.Logger) -> dict[str, Any]:
    notify_characteristic: Any = None

    def on_write(value: list[int], options: Optional[dict] = None) -> None:
        nonlocal notify_characteristic
        frame = bytes(value)
        log.info("BLE command received: %d bytes at %s", len(frame), timestamp())
        response = service.handle_frame(frame)
        if response is None:
            return
        if notify_characteristic is None:
            log.warning("BLE client has not subscribed to notifications")
            return
        notify_characteristic.set_value(list(response))
        log.info("BLE response sent: %d bytes", len(response))

    def on_notify(notifying: bool, characteristic: Any) -> None:
        nonlocal notify_characteristic
        notify_characteristic = characteristic if notifying else None
        log.info("BLE notification subscription: %s", notifying)

    def on_connect(*args: Any) -> None:
        log.info("BLE client connected")

    def on_disconnect(*args: Any) -> None:
        nonlocal notify_characteristic
        notify_characteristic = None
        service.reset_session()
        log.info("BLE client disconnected")

    return {"write": on_write, "notify": on_notify, "connect": on_connect, "disconnect": on_disconnect}


def timestamp() -> str:
    return datetime.now().strftime("%H:%M:%S.%f")[:-3]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--name", default=DEFAULT_LOCAL_NAME, help="BLE 广播设备名称")
    parser.add_argument("--verbose", "-v", action="store_true", help="启用详细日志")
    return parser.parse_args()


def configure_logging(verbose: bool) -> None:
    logging.basicConfig(level=logging.DEBUG if verbose else logging.INFO, format="%(asctime)s [%(levelname)s] %(name)s: %(message)s")


def main() -> int:
    args = parse_args()
    configure_logging(args.verbose)
    log = logging.getLogger("wifi_config_service")
    try:
        from bluezero import adapter, peripheral
    except ImportError:
        log.error("未安装 bluezero，请执行 pip install -r requirements.txt")
        return 127

    adapters = list(adapter.Adapter.available())
    if not adapters:
        log.error("未找到蓝牙适配器")
        return 1
    service = WifiConfigurationService(log)
    callbacks = create_callbacks(service, log)
    device = peripheral.Peripheral(adapters[0].address, local_name=args.name)
    device.on_connect = callbacks["connect"]
    device.on_disconnect = callbacks["disconnect"]
    device.add_service(srv_id=1, uuid=protocol.SERVICE_UUID, primary=True)
    device.add_characteristic(srv_id=1, chr_id=1, uuid=protocol.COMMAND_CHARACTERISTIC_UUID, value=[], notifying=False, flags=["write", "write-without-response"], write_callback=callbacks["write"])
    device.add_characteristic(srv_id=1, chr_id=2, uuid=protocol.NOTIFY_CHARACTERISTIC_UUID, value=[], notifying=False, flags=["read", "notify"], notify_callback=callbacks["notify"])
    log.info("BLE Wi-Fi configuration service started: name=%s", args.name)
    try:
        device.publish()
    except KeyboardInterrupt:
        log.info("service stopped")
    return 0


if __name__ == "__main__":
    sys.exit(main())
