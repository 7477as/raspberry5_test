# BLE Wi-Fi 配网 GATT 服务

> 通过 BLE GATT 为手机或控制器提供 Wi-Fi 工作模式协商和入网配置。

## 功能简介

这个示例在树莓派 5 上提供一个 BLE GATT 服务端，使用固定 UUID 接收客户端的握手、协议协商和 Wi-Fi 模式请求。协议帧采用手写的 Protobuf 风格编码，避免额外生成代码；BLE 服务使用 `bluezero` 对接 BlueZ，适合验证手机 App、控制器与设备之间的配网流程。

## 依赖 / 环境

- 硬件：树莓派 5，板载蓝牙或 USB 蓝牙适配器
- 系统：Raspberry Pi OS Bookworm 64-bit
- 系统库：`sudo apt install bluez python3-gi python3-gi-cairo`
- Python 包：见 `requirements.txt`
- Wi-Fi 工具：NetworkManager (`nmcli`) 或 `wpa_supplicant` (`wpa_cli`)，STA 入网时需要 DHCP 工具

## 构建与运行

### 安装依赖

```bash
cd /home/moyuping/work/raspberry/source/raspberry5_test/python/bluetooth/ble/gatts_wifi_cfg
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
sudo systemctl enable --now bluetooth
```

### 启动 BLE GATT 服务

1. 将 `run.sh` 设置为可执行文件。
2. 双击 `run.sh`，或从终端传入服务端参数。

```bash
cd /home/moyuping/work/raspberry/source/raspberry5_test/python/bluetooth/ble/gatts_wifi_cfg
chmod +x run.sh
./run.sh
./run.sh --name RPi5_WiFi_Config --verbose
```

### 运行协议验证

```bash
cd /home/moyuping/work/raspberry/source/raspberry5_test/python/bluetooth/ble/gatts_wifi_cfg
python3 protocol_validation.py --verbose
python3 ble_client.py --mock --verbose
python3 ble_client.py --addr <BLE地址> --verbose
```

### 验证 Wi-Fi 入网

```bash
cd /home/moyuping/work/raspberry/source/raspberry5_test/python/bluetooth/ble/gatts_wifi_cfg
sudo python3 verify_wifi_connection.py --ssid <SSID> --password <PASSWORD> --gateway <网关IP> --iface wlan0
```

## 关键参数 / 配置

| 参数 / 宏 / 环境变量 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `--name` | `RPi5_WiFi_Config` | BLE 广播设备名称 |
| `--verbose` / `-v` | `False` | 输出详细协议和 BLE 日志 |
| `--mock` | `False` | 不连接 BLE，仅运行客户端协议流程 |
| `--addr` | 空 | 真实 BLE 客户端目标地址 |
| `--iface` | `wlan0` | Wi-Fi STA 网络接口 |
| `--timeout` | `30` | Wi-Fi 入网等待时间，单位为秒 |
| `SERVICE_UUID` | `0xFFA0` | 配网 GATT 服务 UUID |
| `COMMAND_CHARACTERISTIC_UUID` | `0xFFA1` | 客户端写入特征 UUID |
| `NOTIFY_CHARACTERISTIC_UUID` | `0xFFA2` | 服务端通知特征 UUID |

## 已知问题 / 注意事项

- BLE 服务端通常需要 root 权限，`run.sh` 会自动使用 `sudo` 重新启动。
- `bluezero` 依赖 BlueZ、D-Bus 和 PyGObject；如果导入失败，请检查系统库是否安装。
- NetworkManager 和 `wpa_cli` 只能选择一个后端管理 Wi-Fi，混用可能导致连接状态互相覆盖。
- `STA` 模式会在后台执行 Wi-Fi 连接，BLE 请求会先返回协议响应。
- 服务端退出后使用 Ctrl+C；如果蓝牙适配器没有被识别，请检查 `bluetoothctl list`。

## 参考资料

- [BlueZ 官方项目](https://www.bluez.org/)
- [bluezero Python 项目](https://github.com/ukBaz/python-bluezero)
- [Bluetooth GATT 规范](https://www.bluetooth.com/specifications/specs/core-specification/)
- 协议字段说明见 [`PROTOCOL.md`](./PROTOCOL.md)
