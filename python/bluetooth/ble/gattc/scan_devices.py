#!/usr/bin/env python3
"""扫描附近的 BLE 设备并显示完整广播信息。

显示所有设备的详细信息：RSSI、服务 UUID、制造商数据、广播数据等。
"""
from __future__ import annotations

import logging
import sys
from typing import Any

from bluezero import adapter


SCAN_TIMEOUT = 15

KNOWN_MANUFACTURERS = {
    0x004C: "Apple",
    0x0006: "Microsoft",
    0x00E0: "Google",
    0x0075: "Samsung",
    0x0157: "Xiaomi",
    0x038F: "Midea",
    0x0059: "Nordic Semiconductor",
    0x0087: "Qualcomm",
    0x00D2: "Dialog Semiconductor",
    0x0171: "Shenzhen Jingxun Software",
}

KNOWN_SERVICES = {
    "1800": "Generic Access",
    "1801": "Generic Attribute",
    "180a": "Device Information",
    "180f": "Battery Service",
    "1810": "Blood Pressure",
    "1811": "Alert Notification Service",
    "1812": "Human Interface Device",
    "1816": "Cycling Speed and Cadence",
    "181a": "Environmental Sensing",
    "181c": "User Data",
    "181d": "Weight Scale",
}


def setup_logging() -> None:
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)s] %(message)s",
    )


def format_hex_data(data: bytes) -> str:
    """格式化二进制数据为十六进制字符串。"""
    if not data:
        return ""
    hex_str = " ".join(f"{b:02X}" for b in data)
    if len(hex_str) > 60:
        return hex_str[:60] + "..."
    return hex_str


def decode_manufacturer_data(company_id: int, data: bytes) -> str:
    """解码制造商数据。"""
    company_name = KNOWN_MANUFACTURERS.get(company_id, f"未知厂商 (0x{company_id:04X})")
    hex_data = format_hex_data(data)
    
    result = f"{company_name}"
    if hex_data:
        result += f"\n      数据: {hex_data}"
    
    if company_id == 0x004C and len(data) >= 2:
        apple_type = data[0]
        if apple_type == 0x02:
            result += "\n      类型: iBeacon"
        elif apple_type == 0x10:
            result += "\n      类型: Nearby"
        elif apple_type == 0x0C:
            result += "\n      类型: Handoff"
    
    return result


def decode_service_uuid(uuid: str) -> str:
    """解码服务 UUID。"""
    uuid_short = uuid.split("-")[0].lower()
    service_name = KNOWN_SERVICES.get(uuid_short, "")
    
    if service_name:
        return f"{uuid_short.upper()} ({service_name})"
    return uuid


def extract_all_device_info(dev_props: dict) -> dict:
    """提取设备的所有可用信息。"""
    return {
        "address": dev_props.get("Address", ""),
        "name": dev_props.get("Name", ""),
        "alias": dev_props.get("Alias", ""),
        "rssi": dev_props.get("RSSI"),
        "tx_power": dev_props.get("TxPower"),
        "paired": dev_props.get("Paired", False),
        "connected": dev_props.get("Connected", False),
        "trusted": dev_props.get("Trusted", False),
        "blocked": dev_props.get("Blocked", False),
        "address_type": dev_props.get("AddressType", "public"),
        "uuids": dev_props.get("UUIDs", []),
        "service_data": dev_props.get("ServiceData", {}),
        "manufacturer_data": dev_props.get("ManufacturerData", {}),
        "advertise_flags": dev_props.get("AdvertisingFlags"),
        "advertise_data": dev_props.get("AdvertisingData", {}),
        "service_uuids": dev_props.get("ServiceUUIDs", []),
        "raw_props": dev_props,
    }


def get_raw_adv_data(dev_props: dict) -> str:
    """提取广播包的完整原始数据为十六进制字符串。"""
    hex_parts = []
    
    for key in sorted(dev_props.keys()):
        value = dev_props[key]
        
        if isinstance(value, (bytes, bytearray)):
            hex_parts.append(" ".join(f"{b:02X}" for b in value))
        
        elif isinstance(value, (list, tuple)):
            try:
                if len(value) > 0 and isinstance(value[0], int):
                    hex_parts.append(" ".join(f"{b:02X}" for b in value))
            except Exception:
                pass
        
        elif isinstance(value, dict):
            for k, v in value.items():
                if isinstance(v, (bytes, bytearray, list, tuple)):
                    try:
                        data = bytes(v) if isinstance(v, (list, tuple)) else v
                        hex_parts.append(" ".join(f"{b:02X}" for b in data))
                    except Exception:
                        pass
        
        elif isinstance(value, str) and value:
            hex_parts.append(" ".join(f"{ord(c):02X}" for c in value))
    
    return " ".join(hex_parts) if hex_parts else "(无广播数据)"


def print_device_full_info(devices: list[dict]) -> None:
    """打印设备的完整信息。"""
    print("\n" + "=" * 100)
    print(f"共发现 {len(devices)} 个设备\n")
    
    for idx, dev in enumerate(devices, 1):
        separator = "-" * 100
        print(separator)
        print(f"[设备 {idx}] {dev['name'] or dev['alias'] or '未命名设备'}")
        print(separator)
        
        print(f"  地址:        {dev['address']}")
        print(f"  地址类型:    {dev['address_type']}")
        
        if dev['name']:
            print(f"  设备名:      {dev['name']}")
        if dev['alias'] and dev['alias'] != dev['name']:
            print(f"  别名:        {dev['alias']}")
        
        rssi_str = f"{dev['rssi']} dBm" if dev['rssi'] is not None else "N/A"
        signal_quality = ""
        if dev['rssi'] is not None:
            if dev['rssi'] >= -50:
                signal_quality = " (优秀)"
            elif dev['rssi'] >= -60:
                signal_quality = " (良好)"
            elif dev['rssi'] >= -70:
                signal_quality = " (一般)"
            else:
                signal_quality = " (较弱)"
        
        print(f"  信号强度:    {rssi_str}{signal_quality}")
        
        if dev['tx_power'] is not None:
            print(f"  发射功率:    {dev['tx_power']} dBm")
        
        status_parts = []
        if dev['connected']:
            status_parts.append("已连接")
        if dev['paired']:
            status_parts.append("已配对")
        if dev['trusted']:
            status_parts.append("已信任")
        if dev['blocked']:
            status_parts.append("已屏蔽")
        
        if status_parts:
            print(f"  状态:        {', '.join(status_parts)}")
        
        if dev['advertise_flags'] is not None:
            try:
                flag_val = int(dev['advertise_flags'][0]) if isinstance(dev['advertise_flags'], (list, tuple)) else int(dev['advertise_flags'])
                flags = []
                if flag_val & 0x01:
                    flags.append("LE Limited Discoverable")
                if flag_val & 0x02:
                    flags.append("LE General Discoverable")
                if flag_val & 0x04:
                    flags.append("BR/EDR Not Supported")
                if flag_val & 0x08:
                    flags.append("Simultaneous LE and BR/EDR (Controller)")
                if flag_val & 0x10:
                    flags.append("Simultaneous LE and BR/EDR (Host)")
                
                if flags:
                    print(f"  广播标志:    0x{flag_val:02X}")
                    for flag in flags:
                        print(f"               - {flag}")
            except (ValueError, TypeError, IndexError):
                pass
        
        if dev['manufacturer_data']:
            print(f"  制造商数据:")
            for company_id, data in dev['manufacturer_data'].items():
                try:
                    data_bytes = bytes(data) if isinstance(data, (list, tuple)) else data
                    decoded = decode_manufacturer_data(company_id, data_bytes)
                    for line in decoded.split('\n'):
                        print(f"    {line}")
                except (ValueError, TypeError) as e:
                    print(f"    公司ID 0x{company_id:04X}: 无法解析数据")
        
        if dev['uuids']:
            print(f"  服务 UUID:   ({len(dev['uuids'])} 个)")
            for uuid in dev['uuids'][:10]:
                print(f"    - {decode_service_uuid(uuid)}")
            if len(dev['uuids']) > 10:
                print(f"    ... 还有 {len(dev['uuids']) - 10} 个服务")
        
        if dev['service_data']:
            print(f"  服务数据:")
            for service_uuid, data in dev['service_data'].items():
                try:
                    data_bytes = bytes(data) if isinstance(data, (list, tuple)) else data
                    print(f"    UUID: {decode_service_uuid(service_uuid)}")
                    print(f"    数据: {format_hex_data(data_bytes)}")
                except (ValueError, TypeError) as e:
                    print(f"    UUID: {service_uuid} (无法解析数据)")
        
        if dev['advertise_data']:
            print(f"  其他广播数据:")
            for key, value in dev['advertise_data'].items():
                print(f"    {key}: {value}")
        
        # 显示广播包的完整原始十六进制
        raw_hex = get_raw_adv_data(dev['raw_props'])
        print(f"\n  📡 广播包原始数据: {raw_hex}")
        
        print()
    
    print("=" * 100 + "\n")


def main() -> int:
    setup_logging()
    log = logging.getLogger("BLEScan")
    
    adapters = list(adapter.Adapter.available())
    if not adapters:
        log.error("未找到蓝牙适配器")
        return 1
    
    dongle = adapters[0]
    log.info(f"使用蓝牙适配器: {dongle.address}")
    log.info(f"开始扫描设备，超时 {SCAN_TIMEOUT} 秒...")
    
    try:
        dongle.nearby_discovery(timeout=SCAN_TIMEOUT)
        
        devices = []
        mng_objs = adapter.dbus_tools.get_managed_objects()
        
        for path, interfaces in mng_objs.items():
            if adapter.constants.DEVICE_INTERFACE not in interfaces:
                continue
            
            dev_props = interfaces[adapter.constants.DEVICE_INTERFACE]
            dev_info = extract_all_device_info(dev_props)
            devices.append(dev_info)
        
        devices.sort(key=lambda x: x['rssi'] if x['rssi'] is not None else -999, reverse=True)
        
        if not devices:
            log.info("未发现设备")
            return 0
        
        log.info(f"扫描完成")
        print_device_full_info(devices)
        
    except KeyboardInterrupt:
        log.info("扫描被用户中断")
        return 130
    except Exception as e:
        log.exception(f"扫描过程中出错: {e}")
        return 1
    finally:
        try:
            dongle.stop_discovery()
        except Exception:
            pass
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
