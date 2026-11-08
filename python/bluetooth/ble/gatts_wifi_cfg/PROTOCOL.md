# BLE Wi-Fi 配网协议

> 本文定义 BLE GATT Wi-Fi 配网服务使用的 UUID、消息封装、命令和响应字段。

## 1. GATT 通道

| 通道 | UUID | 方向 | 属性 |
| :--- | :--- | :--- | :--- |
| 配网服务 | `0000ffa0-0000-1000-8000-00805f9b34fb` | — | Primary Service |
| 命令特征 | `0000ffa1-0000-1000-8000-00805f9b34fb` | 客户端 → 设备 | Write / Write Without Response |
| 通知特征 | `0000ffa2-0000-1000-8000-00805f9b34fb` | 设备 → 客户端 | Read / Notify |

客户端应先订阅通知特征，再向命令特征写入完整消息帧。

## 2. WsMessage 信封

每个请求和响应都是一个 Protobuf 风格消息。整数使用 varint，字符串和嵌套消息使用 length-delimited 编码。

| 字段号 | 类型 | 请求含义 | 响应含义 |
| :--- | :--- | :--- | :--- |
| `1` | varint | `kind=3` | `kind=1` |
| `2` | string | `request_id` | 原样返回 `request_id` |
| `3` | varint | 事件编号 | 事件编号 |
| `5` | varint | 通常不填写 | `error_code`，0 表示成功 |
| `6` | string | 通常不填写 | 错误说明 |
| `7` | bytes | 命令 payload | 命令响应 payload |
| `8` | varint | `command_id` | 可选 |
| `9` | varint | 客户端来源，Android 为 `1` | 可选 |
| `10` | varint | 协议版本，当前为 `200` | 可选 |

## 3. 命令编号

| 命令 | 编号 | 作用 |
| :--- | :---: | :--- |
| `get_wifi_work_mode` | `70001` | 查询当前 Wi-Fi 工作模式 |
| `set_wifi_work_mode` | `70002` | 请求设备切换 Wi-Fi 工作模式 |
| `wifi_idle_keepalive` | `70005` | 保持 Wi-Fi 模式会话 |
| `protocol_negotiate` | `70007` | 协商协议版本、能力和 MTU |
| `device_handshake` | `70008` | 建立设备会话 |

请求顺序为 `device_handshake` → `protocol_negotiate` → `set_wifi_work_mode`。未完成握手时发送其他命令，服务端返回错误码 `30609`。

## 4. 设备握手

请求 payload：

| 字段号 | 类型 | 含义 |
| :--- | :--- | :--- |
| `1` | varint | 设备角色：手机 `1`，控制器 `2` |
| `2` | string | 客户端系统，例如 `android` |
| `3` | string | 客户端协议或固件版本，例如 `2.0` |
| `4` | string | 客户端设备标识 |

成功响应 payload：

| 字段号 | 类型 | 含义 |
| :--- | :--- | :--- |
| `1` | varint | 服务端接受的角色 |
| `2` | varint | 状态码，0 表示成功 |
| `4` | string | `session_id` |

## 5. 协议协商

请求 payload 字段：`1=client_version`，`2=client_capability`（可重复，支持 `sta`、`softap`、`sta_softap`），`3=client_mtu`。

响应 payload 字段：`1=selected_version`，`2=server_capability`（可重复），`3=server_mtu`，`4=fragmentation_supported`。

## 6. Wi-Fi 工作模式

`set_wifi_work_mode` 请求 payload：

| 字段号 | 类型 | 含义 |
| :--- | :--- | :--- |
| `1` | varint | 目标模式：STA `1`、SOFTAP `2`、STA_SOFTAP `4` |
| `2` | string | 客户端角色，例如 `phone` |
| `3` | string | 客户端系统，例如 `android` |
| `4` | varint | 首选连接类型 |
| `5` | string | STA 目标 SSID |
| `6` | string | STA 目标密码 |
| `7` | varint | `force`，非 0 表示强制切换 |
| `8` | string | 会话 ID |
| `9` | varint | BLE 断开策略：0 保留、1 断开、2 空闲检查 |
| `10` | varint | 空闲策略 |

响应 payload：

| 字段号 | 类型 | 含义 |
| :--- | :--- | :--- |
| `1` | varint | 实际应用模式 |
| `2` | string | 模式说明：`STA` 或 `SoftAP` |
| `3` | bytes | `wifi_conn_info`，见下表 |
| `4` | string | WebSocket 地址 |
| `5` | string | HTTP 地址 |
| `6` | varint | UDP 视频端口 |
| `8` | varint | 是否异步切换 |

`wifi_conn_info` 字段：

| 字段号 | 类型 | 含义 |
| :--- | :--- | :--- |
| `1` | string | SSID |
| `2` | string | 密码 |
| `3` | string | BSSID |
| `4` | varint | Wi-Fi 信道 |
| `5` | varint | 安全类型，WPA2-PSK 为 `1` |
| `6` | string | 客户端 IP |
| `7` | string | 设备 IP |
| `8` | string | 子网掩码 |
| `9` | string | 网关 |

## 7. 错误码

| 错误码 | 含义 |
| :---: | :--- |
| `0` | 成功 |
| `9` | 未知命令 |
| `30608` | 角色槽位已占用 |
| `30609` | 尚未完成握手 |
| `30610` | 无效设备角色 |

## 8. 典型交互

```text
Client -- device_handshake(70008) --> Device
Client <-- response(session_id) ----- Device
Client -- protocol_negotiate(70007) -> Device
Client <-- response(version, mtu) ---- Device
Client -- set_wifi_work_mode(70002) -> Device
Client <-- response(wifi_conn_info) ---- Device
```
