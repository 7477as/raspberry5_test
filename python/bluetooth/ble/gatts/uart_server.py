#!/usr/bin/env python3
"""BLE Nordic UART Service (NUS) Server implementation.

实现 Nordic UART Service 标准协议，提供双向串口透传功能。
"""
from __future__ import annotations

import argparse
import logging
import sys
from datetime import datetime
from typing import Optional

from bluezero import adapter, peripheral
from gi.repository import GLib


NUS_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
NUS_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
NUS_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
DEFAULT_LOCAL_NAME = "RPi5_UART"
DEFAULT_HEARTBEAT_INTERVAL_MS = 5000


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--name",
        type=str,
        default=DEFAULT_LOCAL_NAME,
        help="BLE 广播设备名称",
    )
    parser.add_argument(
        "--heartbeat-interval",
        type=int,
        default=DEFAULT_HEARTBEAT_INTERVAL_MS,
        help="心跳推送间隔（毫秒）",
    )
    parser.add_argument("--verbose", "-v", action="store_true")
    return parser.parse_args()


def setup_logging(verbose: bool = False) -> None:
    logging.basicConfig(
        level=logging.DEBUG if verbose else logging.INFO,
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    )


def get_timestamp() -> str:
    """获取当前时间戳（毫秒精度）"""
    return datetime.now().strftime("%H:%M:%S.%f")[:-3]


def create_uart_handlers(heartbeat_interval_ms: int, log: logging.Logger):
    """创建 UART 处理器的闭包，封装状态"""
    tx_characteristic = None
    timer_id: Optional[int] = None

    def on_rx_write(value: list[int], options: Optional[dict] = None) -> None:
        """RX 写入回调（客户端发送数据到服务端）"""
        log.info(f"[UART RX] 收到客户端数据: {value}")
        try:
            text = bytes(value).decode("utf-8")
            log.info(f"[UART RX] 文本内容: {text}")

            if tx_characteristic:
                echo_msg = f"Echo: {text}"
                tx_characteristic.set_value(list(echo_msg.encode("utf-8")))
                log.info("[UART TX] 自动回显已推送")
        except UnicodeDecodeError:
            log.info("[UART RX] 非文本二进制数据")

    def push_heartbeat(characteristic) -> bool:
        """定时推送心跳数据"""
        msg = f"Ping from RPi5 @ {get_timestamp()}"
        characteristic.set_value(list(msg.encode("utf-8")))
        log.info(f"[UART TX] 心跳推送: {msg}")
        return True

    def on_tx_notify(notifying: bool, characteristic) -> None:
        """TX 通知订阅状态变化回调"""
        nonlocal tx_characteristic, timer_id
        tx_characteristic = characteristic

        if notifying:
            log.info("[STATE] 客户端已订阅 TX 串口通道")
            timer_id = GLib.timeout_add(heartbeat_interval_ms, push_heartbeat, characteristic)
        else:
            log.info("[STATE] 客户端已取消订阅 TX 串口通道")
            if timer_id:
                GLib.source_remove(timer_id)
                timer_id = None
            tx_characteristic = None

    def on_connect(*args) -> None:
        """连接事件回调"""
        log.info("🟢 蓝牙串口连接已建立")

    def on_disconnect(*args) -> None:
        """断开连接事件回调"""
        nonlocal timer_id, tx_characteristic

        log.info("🔴 蓝牙串口连接已断开")
        if timer_id:
            GLib.source_remove(timer_id)
            timer_id = None
        tx_characteristic = None

    return {
        "rx_write": on_rx_write,
        "tx_notify": on_tx_notify,
        "connect": on_connect,
        "disconnect": on_disconnect,
    }


def main() -> int:
    args = parse_args()
    setup_logging(args.verbose)
    log = logging.getLogger("UartServer")

    adapters = list(adapter.Adapter.available())
    if not adapters:
        log.error("未找到可用的蓝牙适配器")
        return 1

    dongle_address = adapters[0].address
    log.info(f"使用蓝牙适配器: {dongle_address}")

    handlers = create_uart_handlers(args.heartbeat_interval, log)

    pi_peripheral = peripheral.Peripheral(dongle_address, local_name=args.name)
    pi_peripheral.on_connect = handlers["connect"]
    pi_peripheral.on_disconnect = handlers["disconnect"]

    pi_peripheral.add_service(srv_id=1, uuid=NUS_SERVICE_UUID, primary=True)

    pi_peripheral.add_characteristic(
        srv_id=1,
        chr_id=1,
        uuid=NUS_RX_CHAR_UUID,
        value=[],
        notifying=False,
        flags=["write", "write-without-response"],
        write_callback=handlers["rx_write"],
    )

    pi_peripheral.add_characteristic(
        srv_id=1,
        chr_id=2,
        uuid=NUS_TX_CHAR_UUID,
        value=[],
        notifying=False,
        flags=["notify"],
        notify_callback=handlers["tx_notify"],
    )

    log.info(f"✅ BLE 串口透传服务器正在运行，设备名称: '{args.name}'")
    log.info("使用 nRF Connect 或支持 Nordic UART 的 App 连接")

    try:
        pi_peripheral.publish()
    except KeyboardInterrupt:
        log.info("程序已退出")
        return 0

    return 0


if __name__ == "__main__":
    sys.exit(main())
