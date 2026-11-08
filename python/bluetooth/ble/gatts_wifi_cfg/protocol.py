#!/usr/bin/env python3
"""BLE Wi-Fi configuration protocol encoding and decoding."""

from __future__ import annotations

from dataclasses import dataclass

SERVICE_UUID = "0000ffa0-0000-1000-8000-00805f9b34fb"
COMMAND_CHARACTERISTIC_UUID = "0000ffa1-0000-1000-8000-00805f9b34fb"
NOTIFY_CHARACTERISTIC_UUID = "0000ffa2-0000-1000-8000-00805f9b34fb"

KIND_EVENT = 2
KIND_REQUEST = 3
KIND_RESPONSE = 1

COMMAND_GET_WIFI_MODE = 70001
COMMAND_SET_WIFI_MODE = 70002
COMMAND_KEEPALIVE = 70005
COMMAND_PROTOCOL_NEGOTIATE = 70007
COMMAND_DEVICE_HANDSHAKE = 70008

WIFI_MODE_STA = 1
WIFI_MODE_SOFTAP = 2
WIFI_MODE_STA_SOFTAP = 4

ROLE_PHONE = 1
ROLE_CONTROLLER = 2

ERROR_OK = 0
ERROR_HANDSHAKE_REQUIRED = 30609
ERROR_ROLE_OCCUPIED = 30608
ERROR_INVALID_ROLE = 30610


@dataclass(frozen=True)
class WifiConnectionInfo:
    ssid: str
    password: str
    bssid: str
    channel: int
    security: int
    client_ip: str
    device_ip: str
    netmask: str
    gateway: str


@dataclass(frozen=True)
class WifiModeRequest:
    target_mode: int
    client_role: str
    client_os: str
    preferred_connection_type: int
    ssid: str
    password: str
    force: bool
    session_id: str
    disconnect_policy: int
    idle_policy: int


def encode_varint(value: int) -> bytes:
    if value < 0:
        raise ValueError("varint cannot encode a negative value")
    result = bytearray()
    while value >= 0x80:
        result.append((value & 0x7F) | 0x80)
        value >>= 7
    result.append(value)
    return bytes(result)


def encode_varint_field(field_number: int, value: int) -> bytes:
    return bytes([(field_number << 3) | 0]) + encode_varint(value)


def encode_bytes_field(field_number: int, value: bytes) -> bytes:
    return bytes([(field_number << 3) | 2]) + encode_varint(len(value)) + value


def encode_string_field(field_number: int, value: str) -> bytes:
    return encode_bytes_field(field_number, value.encode("utf-8"))


def decode_varint(data: bytes, position: int) -> tuple[int, int]:
    value = 0
    shift = 0
    while position < len(data):
        current = data[position]
        position += 1
        value |= (current & 0x7F) << shift
        if not current & 0x80:
            return value, position
        shift += 7
        if shift >= 64:
            raise ValueError("varint is too long")
    raise ValueError("incomplete varint")


def parse_fields(data: bytes) -> dict[int, int | bytes]:
    fields: dict[int, int | bytes] = {}
    position = 0
    while position < len(data):
        tag, position = decode_varint(data, position)
        field_number = tag >> 3
        wire_type = tag & 0x07
        if field_number == 0:
            raise ValueError("protobuf field number cannot be zero")
        if wire_type == 0:
            value, position = decode_varint(data, position)
        elif wire_type == 2:
            length, value_start = decode_varint(data, position)
            value_end = value_start + length
            if value_end > len(data):
                raise ValueError("length-delimited field exceeds frame")
            value = data[value_start:value_end]
            position = value_end
        elif wire_type == 1:
            value = int.from_bytes(data[position:position + 8], "little")
            position += 8
        elif wire_type == 5:
            value = int.from_bytes(data[position:position + 4], "little")
            position += 4
        else:
            raise ValueError(f"unsupported protobuf wire type: {wire_type}")
        fields[field_number] = value
    return fields


def field_string(fields: dict[int, int | bytes], field_number: int) -> str:
    value = fields.get(field_number, b"")
    return value.decode("utf-8", errors="replace") if isinstance(value, bytes) else str(value)


def build_request(command_id: int, request_id: str, payload: bytes = b"") -> bytes:
    frame = encode_varint_field(1, KIND_REQUEST)
    frame += encode_string_field(2, request_id)
    if payload:
        frame += encode_bytes_field(7, payload)
    frame += encode_varint_field(8, command_id)
    frame += encode_varint_field(9, 1)
    frame += encode_varint_field(10, 200)
    return frame


def build_response(request_id: str, error_code: int = 0, payload: bytes = b"") -> bytes:
    frame = encode_varint_field(1, KIND_RESPONSE)
    frame += encode_string_field(2, request_id)
    if error_code:
        frame += encode_varint_field(5, error_code)
    if payload:
        frame += encode_bytes_field(7, payload)
    return frame


def parse_envelope(data: bytes) -> dict[str, int | str | bytes]:
    fields = parse_fields(data)
    return {
        "kind": int(fields.get(1, 0)),
        "request_id": field_string(fields, 2),
        "event_id": int(fields.get(3, 0)),
        "error_code": int(fields.get(5, 0)),
        "error_message": field_string(fields, 6),
        "payload": fields.get(7, b"") if isinstance(fields.get(7, b""), bytes) else b"",
        "command_id": int(fields.get(8, 0)),
    }


def build_handshake_payload(role: int = ROLE_PHONE) -> bytes:
    return (
        encode_varint_field(1, role)
        + encode_string_field(2, "android")
        + encode_string_field(3, "2.0")
        + encode_string_field(4, "configuration-client")
    )


def build_negotiate_payload(mtu: int = 512) -> bytes:
    payload = encode_string_field(1, "2.0")
    payload += encode_string_field(2, "sta")
    payload += encode_string_field(2, "softap")
    payload += encode_string_field(2, "sta_softap")
    payload += encode_varint_field(3, mtu)
    return payload


def build_wifi_mode_payload(mode: int = WIFI_MODE_SOFTAP) -> bytes:
    payload = encode_varint_field(1, mode)
    payload += encode_string_field(2, "phone")
    payload += encode_string_field(3, "android")
    payload += encode_varint_field(9, 2)
    return payload


def parse_wifi_mode_request(payload: bytes) -> WifiModeRequest:
    fields = parse_fields(payload) if payload else {}
    return WifiModeRequest(
        target_mode=int(fields.get(1, 0)),
        client_role=field_string(fields, 2),
        client_os=field_string(fields, 3),
        preferred_connection_type=int(fields.get(4, 0)),
        ssid=field_string(fields, 5),
        password=field_string(fields, 6),
        force=bool(fields.get(7, 0)),
        session_id=field_string(fields, 8),
        disconnect_policy=int(fields.get(9, 0)),
        idle_policy=int(fields.get(10, 0)),
    )


def parse_wifi_connection_info(data: bytes) -> WifiConnectionInfo:
    fields = parse_fields(data) if data else {}
    return WifiConnectionInfo(
        ssid=field_string(fields, 1),
        password=field_string(fields, 2),
        bssid=field_string(fields, 3),
        channel=int(fields.get(4, 0)),
        security=int(fields.get(5, 0)),
        client_ip=field_string(fields, 6),
        device_ip=field_string(fields, 7),
        netmask=field_string(fields, 8),
        gateway=field_string(fields, 9),
    )
