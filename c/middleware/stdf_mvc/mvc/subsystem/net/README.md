# 网络 subject 定义

> 声明网络状态 subject，为网络业务提供数据契约。

## 功能简介

网络层预留 Wi-Fi、Ethernet、MQTT、NTP 状态 ID，供应用与 UI 解耦使用。此目录仅定义数据契约，网络连接由 `app/net/` 扩展；当前未注册网络 payload。

## 依赖 / 环境

- 树莓派 5，Raspberry Pi OS Bookworm 64-bit，C11 和 pthread。
- 构建工具：`sudo apt install gcc cmake pkg-config`。
- 无独立第三方库、模型或资源。

## 构建与运行

随完整示例构建，从当前目录执行：

```bash
cd ../../..
./build_run.sh build
```

## 关键参数 / 配置

| 参数 / 宏 / 环境变量 | 默认值 | 说明 |
| :--- | :--- | :--- |
| 网络 subject ID | `100..103` | Wi-Fi、Ethernet、MQTT、NTP |
| `STDF_MVC_NET_SUBJECT_LIST(X)` | 4 项 | 网络 subject 与名称的二元映射 |
| `STDF_MVC_NET_DATA_LIST(X)` | 空 | 网络 payload 注册，注册后才分配缓存及订阅表 |

在 `stdf_mvc_subject_net.h` 的 subject 清单追加 `X(id, "net.name")`，用于生成枚举和 const 名称表。
在 `stdf_mvc_data_net.h` 定义结构体并追加 `X(id, type)`，用于生成缓存和订阅表；两张表由上级聚合头分别汇总。

## 已知问题 / 注意事项

- 当前网络 ID 只作预留，不接受发布、读取和订阅。
- 模块头只提供注册宏；业务使用全局 subject 枚举时包含 `stdf_mvc_api.h`。

## 参考资料

- [完整构建与主循环派发](../../../README.md)
- [统一注册表](../stdf_mvc_subsystem.h)
