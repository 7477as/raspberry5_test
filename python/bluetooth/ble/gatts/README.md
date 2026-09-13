# BLE GATT Server 示例

> BLE GATT 服务端示例，包含温度传感器服务端和 UART 串口透传服务端。

## 功能简介

本目录包含两个 GATT Server（服务端）示例：

1. **temperature_server.py** - 温度传感器服务端
   - 提供温度读取服务（Read 属性）
   - 支持客户端写入测试数据（Write 属性）
   - 主动推送温度变化（Notify/Indicate 属性）
   - 演示完整的 GATT 特征值属性

2. **uart_server.py** - Nordic UART Service (NUS) 服务端
   - 实现 Nordic UART Service 标准协议
   - RX 通道：接收客户端数据并自动回显
   - TX 通道：定时推送心跳数据
   - 支持双向串口透传

这些服务端可以：
- 被手机 App（nRF Connect / LightBlue）连接和测试
- 与 `gattc/` 目录下的客户端配对通信（Pi to Pi）
- 作为物联网传感器节点的基础模板

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
cd /home/moyuping/work/raspberry/source/raspberry5_test/python/bluetooth/ble/gatts
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

### 运行温度服务端

```bash
python3 temperature_server.py
python3 temperature_server.py --name "MyTempSensor" --interval 3000 -v
```

启动后可以：
1. 使用手机 App（nRF Connect / LightBlue）扫描并连接
2. 使用 `../gattc/temperature_client.py` 客户端连接

### 运行 UART 服务端

```bash
python3 uart_server.py
python3 uart_server.py --name "MyUART" --heartbeat-interval 10000 -v
```

启动后可以：
1. 使用支持 Nordic UART 的手机 App 连接
2. 使用 `../gattc/uart_client.py` 客户端连接

## 关键参数 / 配置

### temperature_server.py

| 参数 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `--name` | `RPi5_Temp_Node` | BLE 广播设备名称 |
| `--interval` | `2000` | 温度检测推送间隔（毫秒） |
| `--verbose` / `-v` | `False` | 启用详细日志 |

**温度数据源：** `/sys/class/thermal/thermal_zone0/temp` (树莓派 CPU 温度)

### uart_server.py

| 参数 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `--name` | `RPi5_UART` | BLE 广播设备名称 |
| `--heartbeat-interval` | `5000` | 心跳推送间隔（毫秒） |
| `--verbose` / `-v` | `False` | 启用详细日志 |

**UART 标准协议：** Nordic UART Service (NUS)
- Service UUID: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- RX Char UUID: `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` (客户端 → 服务端)
- TX Char UUID: `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` (服务端 → 客户端)

## 使用场景

### 手机 App 测试
```bash
# 启动服务端
python3 temperature_server.py

# 使用 nRF Connect (Android/iOS) 或 LightBlue (iOS) 扫描连接
# 设备名称: RPi5_Temp_Node
```

### Pi to Pi 通信
```bash
# 树莓派 A（服务端）
python3 uart_server.py

# 树莓派 B（客户端）
cd ../gattc
python3 uart_client.py
```

### 自定义参数
```bash
# 修改广播名称和推送频率
python3 temperature_server.py --name "Lab_Sensor_01" --interval 1000

# 仅在需要调试时启用详细日志
python3 uart_server.py -v
```

## 已知问题 / 注意事项

- 服务端启动后需要保持运行，按 Ctrl+C 退出
- 连接断开后会自动停止后台推送任务，下次连接会重新启动
- 同一时刻只能有一个客户端连接（BLE 单连接模式）
- 如果广播失败，尝试重启蓝牙：`sudo systemctl restart bluetooth`
- `bluezero` 需要 D-Bus 和 BlueZ 5.x 支持
- 某些 Pi 型号需要先通过 `sudo raspi-config` 启用蓝牙

## GATT 概念说明

### GATT Server vs GATT Client
- **GATT Server (gatts)**: 提供数据和服务的一方（如传感器设备，本目录）
- **GATT Client (gattc)**: 访问数据和服务的一方（如手机 App 或 `../gattc/` 客户端）

### Peripheral vs Central
- **Peripheral（外设）**: 广播自己、等待连接的设备（通常也是 GATT Server）
- **Central（中心）**: 扫描并主动发起连接的设备（通常也是 GATT Client）

**关系对比：**

| 角色 | 行为 | 常见场景 | 本项目对应 |
| :--- | :--- | :--- | :--- |
| Peripheral + GATT Server | 广播 + 提供数据 | 温度传感器、心率带 | `gatts/` 目录（本目录） |
| Central + GATT Client | 扫描 + 读取数据 | 手机 App、Python 客户端 | `gattc/` 目录 |

**注意：** 角色可以独立组合，如某些设备可以同时是 Central + GATT Server。

### GATT 特征值属性

| 属性 | 说明 | 本项目使用 |
| :--- | :--- | :--- |
| Read | 客户端主动读取数据 | `temperature_server.py` 读取温度 |
| Write | 客户端写入数据到服务端 | `temperature_server.py` 接收测试数据 |
| Write Without Response | 写入数据不需要应答 | 两个服务端均支持 |
| Notify | 服务端主动推送数据（无需确认） | 温度变化推送、心跳推送 |
| Indicate | 服务端主动推送数据（需要确认） | `temperature_server.py` 支持 |

## 参考资料

- [bluezero 官方文档](https://github.com/ukBaz/python-bluezero)
- [Nordic UART Service 规范](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/bluetooth_services/services/nus.html)
- [Bluetooth GATT 规范](https://www.bluetooth.com/specifications/specs/core-specification/)
- [树莓派蓝牙配置](https://www.raspberrypi.com/documentation/computers/configuration.html#bluetooth)
