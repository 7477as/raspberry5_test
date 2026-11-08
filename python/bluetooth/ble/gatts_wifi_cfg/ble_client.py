#!/usr/bin/env python3
"""Run the Wi-Fi configuration protocol through a mock or real BLE client."""

from __future__ import annotations

import argparse
import asyncio
import logging
import sys
import time
from typing import Any

import protocol


class BleClient:
    """Connect to the GATT service and exchange protocol frames."""

    def __init__(self, address: str, log: logging.Logger) -> None:
        self.address = address
        self.log = log
        self.client: Any = None
        self.command_characteristic: Any = None
        self.notify_characteristic: Any = None
        self.pending: dict[str, asyncio.Future[dict[str, Any]]] = {}

    async def connect(self) -> bool:
        try:
            from bleak import BleakClient
        except ImportError:
            self.log.error("未安装 bleak，请执行 pip install -r requirements.txt")
            return False
        try:
            self.client = BleakClient(self.address, timeout=10.0)
            await self.client.connect()
            service = self.client.services.get_service(protocol.SERVICE_UUID)
            if service is None:
                raise RuntimeError("未找到配置服务")
            self.command_characteristic = service.get_characteristic(protocol.COMMAND_CHARACTERISTIC_UUID)
            self.notify_characteristic = service.get_characteristic(protocol.NOTIFY_CHARACTERISTIC_UUID)
            if self.command_characteristic is None or self.notify_characteristic is None:
                raise RuntimeError("未找到命令或通知特征")
            await self.client.start_notify(self.notify_characteristic.uuid, self.on_notification)
            return True
        except Exception as error:
            self.log.error("BLE 连接失败：%s", error)
            return False

    def on_notification(self, sender: Any, data: bytearray) -> None:
        message = protocol.parse_envelope(bytes(data))
        request_id = str(message["request_id"])
        future = self.pending.pop(request_id, None)
        if future is not None and not future.done():
            future.set_result(message)

    async def request(self, command_id: int, payload: bytes = b"") -> dict[str, Any]:
        request_id = f"REQ{int(time.time() * 1000) % 100000:05d}"
        frame = protocol.build_request(command_id, request_id, payload)
        future = asyncio.get_running_loop().create_future()
        self.pending[request_id] = future
        await self.client.write_gatt_char(self.command_characteristic.uuid, frame, response=True)
        return await asyncio.wait_for(future, timeout=10)

    async def disconnect(self) -> None:
        if self.client is not None:
            await self.client.disconnect()


class MockService:
    """In-memory service used to validate request and response framing."""

    def __init__(self) -> None:
        self.session_ready = False

    def request(self, frame: bytes) -> bytes:
        message = protocol.parse_envelope(frame)
        command_id = int(message["command_id"])
        request_id = str(message["request_id"])
        if command_id == protocol.COMMAND_DEVICE_HANDSHAKE:
            self.session_ready = True
            payload = protocol.encode_varint_field(1, protocol.ROLE_PHONE)
            payload += protocol.encode_varint_field(2, 0)
            payload += protocol.encode_string_field(4, "MOCK_SESSION")
            return protocol.build_response(request_id, payload=payload)
        if not self.session_ready:
            return protocol.build_response(request_id, protocol.ERROR_HANDSHAKE_REQUIRED)
        if command_id == protocol.COMMAND_PROTOCOL_NEGOTIATE:
            payload = protocol.encode_string_field(1, "2.0")
            payload += protocol.encode_varint_field(3, 512)
            return protocol.build_response(request_id, payload=payload)
        if command_id == protocol.COMMAND_SET_WIFI_MODE:
            return protocol.build_response(request_id, payload=protocol.encode_varint_field(1, protocol.WIFI_MODE_SOFTAP))
        return protocol.build_response(request_id)


def run_mock() -> bool:
    service = MockService()
    requests = [
        protocol.build_request(protocol.COMMAND_DEVICE_HANDSHAKE, "MOCK001", protocol.build_handshake_payload()),
        protocol.build_request(protocol.COMMAND_PROTOCOL_NEGOTIATE, "MOCK002", protocol.build_negotiate_payload()),
        protocol.build_request(protocol.COMMAND_SET_WIFI_MODE, "MOCK003", protocol.build_wifi_mode_payload()),
    ]
    for request in requests:
        response = protocol.parse_envelope(service.request(request))
        if response["error_code"] != 0:
            return False
    return True


async def run_ble(address: str, log: logging.Logger) -> bool:
    client = BleClient(address, log)
    if not await client.connect():
        return False
    try:
        commands = [
            (protocol.COMMAND_DEVICE_HANDSHAKE, protocol.build_handshake_payload()),
            (protocol.COMMAND_PROTOCOL_NEGOTIATE, protocol.build_negotiate_payload()),
            (protocol.COMMAND_SET_WIFI_MODE, protocol.build_wifi_mode_payload()),
        ]
        for command_id, payload in commands:
            response = await client.request(command_id, payload)
            if response["error_code"] != 0:
                return False
        return True
    finally:
        await client.disconnect()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mock", action="store_true", help="只运行内存协议验证")
    parser.add_argument("--addr", default="", help="BLE 设备地址")
    parser.add_argument("--verbose", "-v", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")
    log = logging.getLogger("ble_client")
    if args.mock:
        passed = run_mock()
        print("BLE 客户端协议验证通过" if passed else "BLE 客户端协议验证失败")
        return 0 if passed else 1
    if not args.addr:
        log.error("请提供 --addr，或使用 --mock")
        return 1
    return 0 if asyncio.run(run_ble(args.addr, log)) else 1


if __name__ == "__main__":
    sys.exit(main())
