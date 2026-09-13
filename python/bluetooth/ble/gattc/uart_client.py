#!/usr/bin/env python3
"""BLE GATT Client for Nordic UART Service.

连接到 BLE UART 服务端，实现双向串口透传通信。
"""
from __future__ import annotations

import argparse
import logging
import sys
import threading
from pathlib import Path

from bluezero import adapter, central


NUS_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
NUS_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
NUS_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
TARGET_DEVICE_NAME = "RPi5_UART"


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


def on_uart_rx(value: list[int]) -> None:
    """从服务端 TX 特征值接收数据（服务端推送过来的数据）"""
    log = logging.getLogger("UartClient")
    try:
        text = bytes(value).decode("utf-8")
        log.info(f"📥 收到服务端消息: {text}")
    except UnicodeDecodeError:
        hex_str = " ".join([f"{b:02X}" for b in value])
        log.warning(f"收到二进制数据: {hex_str}")


def input_thread(device: central.Central, stop_event: threading.Event) -> None:
    """独立线程处理用户输入，发送到服务端"""
    log = logging.getLogger("UartClient.Input")
    log.info("输入线程已启动，输入文本后回车发送（输入 'exit' 退出）")
    
    while not stop_event.is_set():
        try:
            user_input = input()
            if user_input.lower() in ("exit", "quit"):
                log.info("用户请求退出")
                stop_event.set()
                break
            
            if user_input.strip():
                data = list(user_input.encode("utf-8"))
                rx_char = None
                for char in device._characteristics:
                    if char.chrc_uuid == NUS_RX_CHAR_UUID:
                        rx_char = char
                        break
                if rx_char:
                    rx_char.write_value(data)
                    log.info(f"📤 已发送: {user_input}")
                else:
                    log.error("未找到 RX 特征值")
        except EOFError:
            break
        except Exception as e:
            log.error(f"发送失败: {e}")


def main() -> int:
    args = parse_args()
    setup_logging(args.verbose)
    log = logging.getLogger("UartClient")

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
    device.add_characteristic(NUS_SERVICE_UUID, NUS_RX_CHAR_UUID)
    device.add_characteristic(NUS_SERVICE_UUID, NUS_TX_CHAR_UUID)
    device.connect(timeout=args.timeout)
    
    log.info(f"✅ 已连接到 BLE UART 设备: {target_device.address}")

    stop_event = threading.Event()

    try:
        log.info("订阅 TX 通道（接收服务端推送的数据）...")
        tx_char = None
        for char in device._characteristics:
            if char.chrc_uuid == NUS_TX_CHAR_UUID:
                tx_char = char
                break
        
        if tx_char:
            tx_char.add_characteristic_cb(on_uart_rx)
            tx_char.start_notify()
            log.info("📡 已开启 UART RX 监听")
        else:
            log.error("未找到 TX 特征值")
            return 1

        log.info("启动交互式输入线程...")
        input_thd = threading.Thread(target=input_thread, args=(device, stop_event), daemon=True)
        input_thd.start()

        log.info("🔗 BLE 串口透传已就绪，等待收发消息...")
        while not stop_event.is_set():
            stop_event.wait(0.5)

    except KeyboardInterrupt:
        log.info("\n用户中断")
    except Exception as e:
        log.exception(f"发生错误: {e}")
        return 1
    finally:
        stop_event.set()
        try:
            for char in device._characteristics:
                if char.chrc_uuid == NUS_TX_CHAR_UUID:
                    char.stop_notify()
                    break
            device.disconnect()
            log.info("已断开连接")
        except Exception:
            pass

    return 0


if __name__ == "__main__":
    sys.exit(main())
