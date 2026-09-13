#!/usr/bin/env python3
"""BLE GATT Temperature Server with all properties demonstration.

演示完整的 GATT 特征值属性：Read, Write, Notify, Indicate。
"""
from __future__ import annotations

import argparse
import logging
import sys
from datetime import datetime
from typing import Optional

from bluezero import adapter, peripheral
from gi.repository import GLib


TEMP_SERVICE_UUID = "12341000-1234-1234-1234-123456789abc"
TEMP_CHAR_UUID = "23452000-1234-1234-1234-123456789abc"
DEFAULT_LOCAL_NAME = "RPi5_Temp_Node"
DEFAULT_NOTIFY_INTERVAL_MS = 2000


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--name",
        type=str,
        default=DEFAULT_LOCAL_NAME,
        help="BLE 广播设备名称",
    )
    parser.add_argument(
        "--interval",
        type=int,
        default=DEFAULT_NOTIFY_INTERVAL_MS,
        help="温度检测间隔（毫秒）",
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


def create_temp_handlers(interval_ms: int, log: logging.Logger):
    """创建温度处理器的闭包，封装状态"""
    timer_id: Optional[int] = None
    last_temp: Optional[float] = None

    def read_temperature() -> list[int]:
        """读取温度回调（Read 属性）"""
        try:
            with open("/sys/class/thermal/thermal_zone0/temp", "r") as f:
                temp_c = int(f.read().strip()) / 1000.0
        except Exception:
            temp_c = 0.0

        log.info(f"[READ] 客户端读取温度: {temp_c}°C")
        return list(f"{temp_c:.1f}".encode("utf-8"))

    def write_data(value: list[int], options: Optional[dict] = None) -> None:
        """写入数据回调（Write 属性）"""
        log.info(f"[WRITE] 收到原始数据: {value}")
        if options:
            log.debug(f"[WRITE] 附加选项: {options}")

        try:
            decoded_value = bytes(value).decode("utf-8")
            log.info(f"[WRITE] 解码为字符串: {decoded_value}")
        except UnicodeDecodeError:
            hex_str = " ".join([f"{b:02X}" for b in value])
            log.info(f"[WRITE] 十六进制数据: {hex_str}")

    def push_temperature_data(characteristic) -> bool:
        """定时推送温度数据（Notify/Indicate 属性）"""
        nonlocal last_temp
        try:
            with open("/sys/class/thermal/thermal_zone0/temp", "r") as f:
                temp_c = int(f.read().strip()) / 1000.0
        except Exception:
            temp_c = 0.0

        if temp_c != last_temp:
            temp_bytes = list(f"{temp_c:.1f}".encode("utf-8"))
            log.info(f"[NOTIFY] 温度变化，主动推送: {temp_c}°C")
            characteristic.set_value(temp_bytes)
            last_temp = temp_c

        return True

    def notify_callback(notifying: bool, characteristic) -> None:
        """通知订阅状态变化回调"""
        nonlocal timer_id, last_temp

        if notifying:
            log.info("[STATE] 客户端已开启 Notify/Indicate 订阅")
            last_temp = None
            timer_id = GLib.timeout_add(interval_ms, push_temperature_data, characteristic)
        else:
            log.info("[STATE] 客户端已取消订阅")
            if timer_id:
                GLib.source_remove(timer_id)
                timer_id = None

    def on_connect(*args) -> None:
        """连接事件回调"""
        log.info("🟢 蓝牙连接已建立")
        for arg in args:
            if hasattr(arg, "address"):
                log.info(f"远端设备 MAC: {arg.address}")

    def on_disconnect(*args) -> None:
        """断开连接事件回调"""
        nonlocal timer_id, last_temp

        log.info("🔴 蓝牙连接已断开")
        if timer_id:
            GLib.source_remove(timer_id)
            timer_id = None
            log.info("[STATE] 已停止后台推送任务")

        last_temp = None

    return {
        "read": read_temperature,
        "write": write_data,
        "notify": notify_callback,
        "connect": on_connect,
        "disconnect": on_disconnect,
    }


def print_adv_packet_info(local_name: str, service_uuid: str, log: logging.Logger) -> None:
    """打印模拟的 BLE 广播包信息"""
    log.info("========= BLE 广播包分析 (31 Byte) =========")
    flags = "02 01 06"
    log.info(f"Flags:  {flags}")

    name_bytes = local_name.encode("utf-8")
    name_hex = " ".join([f"{b:02X}" for b in name_bytes])
    log.info(f"Name:   {len(name_bytes)+1:02X} 09 {name_hex} ('{local_name}')")

    uuid_raw = service_uuid.replace("-", "")
    uuid_hex = " ".join([uuid_raw[i : i + 2] for i in range(0, len(uuid_raw), 2)][::-1])
    log.info(f"UUID:   11 07 {uuid_hex}")
    log.info("=" * 44)


def main() -> int:
    args = parse_args()
    setup_logging(args.verbose)
    log = logging.getLogger("TempServer")

    adapters = list(adapter.Adapter.available())
    if not adapters:
        log.error("未找到可用的蓝牙适配器")
        return 1

    dongle_address = adapters[0].address
    log.info(f"使用蓝牙适配器: {dongle_address}")

    handlers = create_temp_handlers(args.interval, log)

    pi_peripheral = peripheral.Peripheral(dongle_address, local_name=args.name)
    pi_peripheral.on_connect = handlers["connect"]
    pi_peripheral.on_disconnect = handlers["disconnect"]

    pi_peripheral.add_service(srv_id=1, uuid=TEMP_SERVICE_UUID, primary=True)
    pi_peripheral.add_characteristic(
        srv_id=1,
        chr_id=1,
        uuid=TEMP_CHAR_UUID,
        value=[],
        notifying=False,
        flags=["read", "write", "write-without-response", "notify", "indicate"],
        read_callback=handlers["read"],
        write_callback=handlers["write"],
        notify_callback=handlers["notify"],
    )

    print_adv_packet_info(args.name, TEMP_SERVICE_UUID, log)

    log.info(f"✅ BLE 温度服务器正在运行，设备名称: '{args.name}'")
    log.info("使用 nRF Connect 或 LightBlue 扫描连接")

    try:
        pi_peripheral.publish()
    except KeyboardInterrupt:
        log.info("程序已退出")
        return 0

    return 0


if __name__ == "__main__":
    sys.exit(main())
