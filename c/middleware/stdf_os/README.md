# stdf_os

> 树莓派 5 上 stdf OS 子系统的最小可运行示例：mailbox + 延迟消息 + 自重链定时器。

## 功能简介

| 模块 | 演示内容 |
| :--- | :--- |
| `stdf_app_key` | pthread 模拟 PRESS/RELEASE，状态机识别 SINGLE / DOUBLE / TRIPLE / LONG |
| `stdf_app_key_sim` | pthread 持续产生 4 种按键时间序列，作为物理输入源 |
| `stdf_app_heartbeat` | 1 秒一拍心跳，handler 内 `message_send_later` 自重链 |

OS 平台适配（pthread / `timer_create` / mutex / `clock_gettime`）集中在 `stdf_os/stdf_os_port.{c,h}`，换平台只需改这一个文件。app 层完全感知不到 pthread 存在。

适用：作为 stdf 风格代码的最小可跑 demo，方便阅读 `stdf_bsp_key` 类状态机实现前的过渡。

## 依赖 / 环境

- 硬件：树莓派 5 或任意 glibc ≥ 2.31 的 Linux 主机
- 系统：`Raspberry Pi OS Bookworm 64-bit`
- 系统库：`sudo apt install cmake build-essential`
- 可选：`sudo apt install libgpiod-dev`（未装时自动用 pthread fallback）

## 构建与运行

```bash
cd c/middleware/stdf_os
chmod +x build_run.sh
./build_run.sh          # 默认：配置 + 构建 + 运行
./build_run.sh build    # 仅构建
./build_run.sh clean    # 清理 build/
./build_run.sh rebuild  # clean + build
NORUN=1 ./build_run.sh  # 仅构建不运行
BUILD_TYPE=Debug ./build_run.sh build
```

`Ctrl+C` 干净退出（所有 pthread 释放）。

## 关键参数 / 配置

| 宏 | 默认 | 所在文件 | 说明 |
| :--- | :--- | :--- | :--- |
| `STDF_APP_KEY_MULT_PRESS_TIME_MS` | `500` | `stdf_app_key.c` | 多击间隔阈值 |
| `STDF_APP_KEY_LONG_PRESS_TIME_MS` | `1500` | `stdf_app_key.c` | 长按阈值 |
| `STDF_APP_HEARTBEAT_PERIOD_MS` | `1000` | `stdf_app_heartbeat.c` | 心跳周期 |
| `STDF_OS_MSG_MAILBOX_MAX` | `30` | `stdf_os_config.h` | mailbox 队列深度 |
| `STDF_OS_DELAY_MSG_MAX_NUM` | `20` | `stdf_os_config.h` | 延迟定时器池大小 |
| `STDF_OS_MSG_THREAD_STACK_SIZE` | `4 KB` | `stdf_os_config.h` | 后台消息线程栈 |

## 关键日志

- `[STDF][I] stdf_app_heartbeat_msg_handler heartbeat` — 心跳每秒一拍
- `[USER] PRESS / RELEASE / SINGLE_CLICK / DOUBLE_CLICK / TRIPLE_CLICK / LONG_PRESS` — 识别出的按键事件
- 默认 `INFO` 级；要看 OS 内部 `add / timer_timeout` trace，用 `BUILD_TYPE=Debug ./build_run.sh rebuild`

## 已知问题 / 注意事项

- **无真实硬件输入**：按键完全靠 pthread 模拟时间序列；要测真 GPIO，把 `stdf_app_key.c` 里的 `sim_press/release` 换成 `gpiod` 边沿事件即可
- **libgpiod 可选**：未装 `libgpiod-dev` 时 `stdf_app_key` 用 pthread fallback（当前默认）
- **macOS**：`timer_create` 行为不同，主体仍能跑但计时不准
- **demo 永不自然停止**：`heartbeat` 持续不停，按 `Ctrl+C` 退出

## 参考资料

- [`.cursor/rules/11-c-stdf.mdc`](../../../.cursor/rules/11-c-stdf.mdc) —— 本仓库 stdf 风格规则
- `bes2600/BES2600IHC/stdf/stdf_bsp/key/stdf_bsp_key.{c,h}` —— 按键状态机参考样板
