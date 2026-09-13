#!/usr/bin/env python3
"""BLE GATT Client for Temperature Service.

连接到温度传感器 GATT Server，读取温度并订阅通知。
"""
from __future__ import annotations

import argparse
import logging
import sys
import time
from pathlib import Path

from bluezero import adapter, central


TEMP_SERVICE_UUID = "12341000-1234-1234-1234-123456789abc"
TEMP_CHAR_UUID = "23452000-1234-1234-1234-123456789abc"
TARGET_DEVICE_NAME = "RPi5_Temp_Node"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--device-name",
        type=str,
        default=TARGET_DEVICE_NAME,
        help="目标设备的广播名称",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=30,
        help="扫描超时时间（秒）",
    )
    parser.add_argument("--verbose", "-v", action="store_true")
    return parser.parse_args()


def setup_logging(verbose: bool = False) -> None:
    logging.basicConfig(
        level=logging.DEBUG if verbose else logging.INFO,
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    )


def on_notify(value: list[int]) -> None:
    """温度特征值变化时的回调函数"""
    log = logging.getLogger("TempClient")
    try:
        temp_str = bytes(value).decode("utf-8")
        log.info(f"🌡️  收到推送的温度数据: {temp_str}°C")
    except UnicodeDecodeError:
        hex_str = " ".join([f"{b:02X}" for b in value])
        log.warning(f"收到非文本数据: {hex_str}")


def main() -> int:
    args = parse_args()
    setup_logging(args.verbose)
    log = logging.getLogger("TempClient")

    adapters = list(adapter.Adapter.available())
    if not adapters:
        log.error("未找到可用的蓝牙适配器")
        return 1

    dongle = adapters[0]
    log.info(f"使用蓝牙适配器: {dongle.address}")

    log.info(f"开始扫描设备 '{args.device_name}'，超时 {args.timeout} 秒...")
    
    dongle.nearby_discovery(timeout=args.timeout)
    
    target_device = None
    for dev in central.Central.available(dongle.address):
        if dev.name == args.device_name:
            target_device = dev
            break
    
    if not target_device:
        log.error(f"未找到设备 '{args.device_name}'")
        return 1

    log.info(f"找到设备: {target_device.address}，正在连接...")
    
    device = central.Central(target_device.address, dongle.address)
    device.add_characteristic(TEMP_SERVICE_UUID, TEMP_CHAR_UUID)
    device.connect(timeout=args.timeout)
    
    log.info(f"✅ 已连接到 BLE 温度传感器: {target_device.address}")

    try:
        log.info("正在读取温度特征值...")
        temp_char = None
        for char in device._characteristics:
            if char.chrc_uuid == TEMP_CHAR_UUID:
                temp_char = char
                break
        
        if not temp_char:
            log.error("未找到温度特征值")
            return 1
        
        temp_value = temp_char.read_value()
        temp_str = bytes(temp_value).decode("utf-8")
        log.info(f"📖 读取到的温度: {temp_str}°C")

        log.info("发送测试数据到服务端...")
        test_data = "Hello from Client"
        temp_char.write_value(list(test_data.encode("utf-8")))
        log.info(f"✍️  已写入: {test_data}")

        log.info("订阅温度变化通知...")
        temp_char.add_characteristic_cb(on_notify)
        temp_char.start_notify()
        log.info("📡 已开启 Notify 订阅")

        log.info("持续监听中（Ctrl+C 退出）...")
        while True:
            time.sleep(1)

    except KeyboardInterrupt:
        log.info("\n用户中断")
    except Exception as e:
        log.exception(f"发生错误: {e}")
        return 1
    finally:
        try:
            temp_char = None
            for char in device._characteristics:
                if char.chrc_uuid == TEMP_CHAR_UUID:
                    temp_char = char
                    break
            if temp_char:
                temp_char.stop_notify()
            device.disconnect()
            log.info("已断开连接")
        except Exception:
            pass

    return 0


if __name__ == "__main__":
    sys.exit(main())
