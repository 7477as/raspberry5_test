# network 子系统（纯数据层）

> 纯数据层：仅定义 subject ID 枚举与 payload 结构体（无任何业务逻辑）。后续扩展 WiFi/BLE/RTMP/MQTT/NTP 等网络相关发布。

## subject 定义

| ID | 名称 | payload 大小 |
|---|---|---|
| `STDF_MVC_SUBJECT_NET_WIFI_STATE` | `net.wifi_state` | 0（占位） |
| `STDF_MVC_SUBJECT_NET_ETH_STATE` | `net.eth_state` | 0（占位） |
| `STDF_MVC_SUBJECT_NET_MQTT_STATE` | `net.mqtt_state` | 0（占位） |
| `STDF_MVC_SUBJECT_NET_NTP_SYNC` | `net.ntp_sync` | 0（占位） |

## 文件

- `std_mvc_subject_net.h` —— subject ID 枚举
- `std_mvc_data_net.h` —— payload 结构体（当前为空）
