#!/usr/bin/env python3
"""Validate the complete Wi-Fi configuration protocol without BLE hardware."""

from __future__ import annotations

import argparse
import logging
import sys

from main import WifiConfigurationService
import protocol


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verbose", "-v", action="store_true")
    return parser.parse_args()


def configure_logging(verbose: bool) -> None:
    logging.basicConfig(level=logging.DEBUG if verbose else logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")


def validate_handshake(service: WifiConfigurationService) -> str:
    request = protocol.build_request(protocol.COMMAND_DEVICE_HANDSHAKE, "VALIDATE001", protocol.build_handshake_payload())
    response = service.handle_frame(request)
    assert response is not None
    message = protocol.parse_envelope(response)
    assert message["error_code"] == protocol.ERROR_OK
    fields = protocol.parse_fields(bytes(message["payload"]))
    session_id = protocol.field_string(fields, 4)
    assert session_id
    return session_id


def validate_negotiation(service: WifiConfigurationService) -> None:
    request = protocol.build_request(protocol.COMMAND_PROTOCOL_NEGOTIATE, "VALIDATE002", protocol.build_negotiate_payload())
    response = service.handle_frame(request)
    assert response is not None
    message = protocol.parse_envelope(response)
    assert message["error_code"] == protocol.ERROR_OK
    fields = protocol.parse_fields(bytes(message["payload"]))
    assert protocol.field_string(fields, 1) == "2.0"
    assert fields[3] == 512


def validate_wifi_mode(service: WifiConfigurationService) -> None:
    request = protocol.build_request(protocol.COMMAND_SET_WIFI_MODE, "VALIDATE003", protocol.build_wifi_mode_payload())
    response = service.handle_frame(request)
    assert response is not None
    message = protocol.parse_envelope(response)
    assert message["error_code"] == protocol.ERROR_OK
    fields = protocol.parse_fields(bytes(message["payload"]))
    assert fields[1] == protocol.WIFI_MODE_SOFTAP
    info = protocol.parse_wifi_connection_info(bytes(fields[3]))
    assert info.device_ip == "192.168.4.1"
    assert fields[6] == 50062


def main() -> int:
    args = parse_args()
    configure_logging(args.verbose)
    service = WifiConfigurationService(logging.getLogger("protocol_validation"))
    session_id = validate_handshake(service)
    validate_negotiation(service)
    validate_wifi_mode(service)
    print(f"协议验证通过，session_id={session_id}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
