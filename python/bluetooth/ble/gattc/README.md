# BLE GATT Client 示例

> BLE GATT 客户端示例，包含温度传感器客户端和 UART 串口透传客户端。

## 功能简介

本目录包含两个 GATT Client（客户端）示例：

1. **temperature_client.py** - 温度传感器客户端
   - 连接到 `gatts/ble_all_properties.py` 服务端
   - 读取温度数据
   - 订阅温度变化通知
   - 发送测试数据

2. **uart_client.py** - Nordic UART Service (NUS) 客户端
   - 连接到 `gatts/ble_uart.py` 服务端
   - 实现双向串口透传
   - 支持交互式输入发送
   - 实时接收服务端推送的消息

与使用手机 App（nRF Connect / LightBlue）不同，这些客户端可以：
- 在树莓派之间直接通信（Pi to Pi）
- 自动化测试 GATT 服务
- 集成到自动化脚本中

## 依赖 / 环境

- 硬件：树莓派 5（或其他支持 BLE 的树莓派）
- 系统：Raspberry Pi OS Bookworm 64-bit
- 系统库：
  ```bash
  sudo apt install python3-gi python3-gi-cairo gir1.2-gtk-3.0 bluez
  ```
- Python 包：见 `requirements.txt`

## 构建与运行

### 前提条件

确保蓝牙服务运行正常：
```bash
sudo systemctl status bluetooth
sudo bluetoothctl power on
```

### 安装依赖

```bash
cd /home/moyuping/work/raspberry/source/raspberry5_test/python/bluetooth/ble/gattc
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

### 运行温度客户端

先在另一台树莓派上启动温度服务端：
```bash
cd ../gatts
python3 ble_all_properties.py
```

然后运行客户端：
```bash
python3 temperature_client.py
python3 temperature_client.py --device-name "RPi5_Temp_Node" --timeout 30 -v
```

### 运行 UART 客户端

先启动 UART 服务端：
```bash
cd ../gatts
python3 ble_uart.py
```

然后运行客户端：
```bash
python3 uart_client.py
python3 uart_client.py --device-name "RPi5_UART" -v
```

客户端连接成功后，可以直接输入文本并回车发送，输入 `exit` 退出。

## 关键参数 / 配置

### temperature_client.py

| 参数 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `--device-name` | `RPi5_Temp_Node` | 要连接的服务端设备名称 |
| `--timeout` | `30` | 扫描超时时间（秒） |
| `--verbose` / `-v` | `False` | 启用详细日志 |

### uart_client.py

| 参数 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `--device-name` | `RPi5_UART` | 要连接的服务端设备名称 |
| `--timeout` | `30` | 扫描超时时间（秒） |
| `--verbose` / `-v` | `False` | 启用详细日志 |

## 使用场景

### Pi to Pi 通信
```bash
# 树莓派 A（服务端）
python3 ../gatts/ble_uart.py

# 树莓派 B（客户端）
python3 uart_client.py
```

### 自动化测试
```python
# 在脚本中调用
import subprocess
result = subprocess.run(
    ["python3", "temperature_client.py", "--timeout", "10"],
    capture_output=True
)
```

## 已知问题 / 注意事项

- 确保服务端已启动并正在广播，否则客户端扫描会超时
- 两台设备需要在蓝牙信号范围内（通常 10 米以内）
- 如果连接失败，尝试重启蓝牙：`sudo systemctl restart bluetooth`
- `bluezero` 需要 D-Bus 和 BlueZ 5.x 支持
- 某些 Pi 型号需要先通过 `sudo raspi-config` 启用蓝牙

## GATT 概念说明

### GATT Client vs GATT Server
- **GATT Server (gatts)**: 提供数据和服务的一方（如传感器设备）
- **GATT Client (gattc)**: 访问数据和服务的一方（如手机 App 或本客户端）

### Peripheral vs Central
- **Peripheral（外设）**: 广播自己、等待连接的设备（通常也是 GATT Server）
- **Central（中心）**: 扫描并主动发起连接的设备（通常也是 GATT Client）

**关系对比：**

| 角色 | 行为 | 常见场景 | 本项目对应 |
| :--- | :--- | :--- | :--- |
| Peripheral + GATT Server | 广播 + 提供数据 | 温度传感器、心率带 | `gatts/` 目录 |
| Central + GATT Client | 扫描 + 读取数据 | 手机 App、本客户端 | `gattc/` 目录 |

**注意：** 角色可以独立组合，如某些设备可以同时是 Central + GATT Server。

## 参考资料

- [bluezero 官方文档](https://github.com/ukBaz/python-bluezero)
- [Nordic UART Service 规范](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/bluetooth_services/services/nus.html)
- [Bluetooth GATT 规范](https://www.bluetooth.com/specifications/specs/core-specification/)
