# STDF MVC 发布订阅框架

> C11 发布订阅示例，展示树莓派 5 系统状态采集、发布与 UI 订阅。

## 功能简介

STDF MVC 让 CPU、内存、设备、电源、存储和网络状态采集模块各自独立发布，UI 根据主题订阅并显示变化。状态主题仅缓存最新值，适合监控面板，不保证逐条消息交付。

## 依赖 / 环境

- 硬件：树莓派 5；网络状态可在其他 Linux 主机运行。
- 系统：Raspberry Pi OS 64-bit 或兼容 Linux。
- 系统库：`sudo apt install build-essential cmake pkg-config libsdl2-dev libraspberrypi-bin iw`
- 电源数据：需要 `vcgencmd pmic_read_adc`、`vcgencmd get_throttled` 和 `/dev/vcio` 可访问权限；不支持时其余系统状态仍可用。
- Wi-Fi 信号：需要 `iw` 命令读取真实 dBm；缺少时退化为 `/proc/net/wireless` 的原始质量值。
- UI：安装 SDL2 开发库时构建图形面板；否则使用终端状态输出。
- Python 包：不需要。
- 模型 / 资源：不需要。

## 构建与运行

```bash
cd c/middleware/stdf_mvc
cmake -S . -B build -DSTDF_MVC_ENABLE_SDL2=ON
cmake --build build -j4
./build/stdf_mvc_demo
```

关闭 SDL2 使用终端状态输出：

```bash
cmake -S . -B build-headless -DSTDF_MVC_ENABLE_SDL2=OFF
cmake --build build-headless -j4
./build-headless/stdf_mvc_demo
```

SDL 面板按 `1` 切换总览、`2` 查看网络接口、`3` 查看 PMIC 电源、`4` 查看存储；上下方向键或滚轮滚动列表，`Esc` 退出。

## 关键参数 / 配置

| 参数 / 宏 / 环境变量 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `STDF_MVC_ENABLE_SDL2` | `ON` | SDL2 可用时构建图形状态面板 |
| CPU 采样周期 | `1 秒` | 温度、频率、CPU 总使用率 |
| 内存采样周期 | `2 秒` | MemTotal、MemAvailable、使用率 |
| 设备信息采样周期 | `1 秒` | 启动时长与静态设备信息 |
| 电源采样周期 | `5 秒` | PMIC 电压、电流、欠压降频标志与风扇转速 |
| 存储采样周期 | `10 秒` | 根分区容量与可用空间 |
| 网络采样周期 | `1 秒` | 网卡类型、链路、IPv4、收发速率及 Wi-Fi 信号 |

## 已知问题 / 注意事项

- Pi 5 PMIC 可读取 5V 输入电压与各内部供电轨的电压、电流，但不提供整机 5V 输入电流；不能把各电压轨电流简单相加作为输入电流。
- 电源读取需要 `vcgencmd` 可执行且 `/dev/vcio` 权限允许。权限或固件不可用时，UI 显示 `N/A`。
- 风扇转速依赖带测速输出的风扇和 `hwmon` 节点；普通树莓派 5 官方散热器无测速时会显示 `N/A`。
- CPU 使用率基于 `/proc/stat` 计数差，首次采样没有基线，因此显示 `N/A`，随后采样产生有效读数。
- 网络收发速率首次采样、网卡重建或计数器回退时显示 `N/A`。
- Wi-Fi 信号优先取 `iw` 的 dBm；`iw` 不可用时退化为 `/proc/net/wireless` 原始质量值并标记 `raw`。
- IPv4 地址优先用 `getifaddrs()`，失败时回退到 netlink 路由套接字。
- 状态主题保留最新数据，发布频率高于主循环派发频率时中间值可能被覆盖。
- 当前 camera 模块仍是占位初始化示例。

## 参考资料

- [Raspberry Pi OS `vcgencmd` 文档](https://www.raspberrypi.com/documentation/computers/os.html#vcgencmd)
- [Linux `/proc` 文件系统](https://docs.kernel.org/filesystems/proc.html)
- [STDF MVC API](./mvc/stdf_mvc_api.h)
