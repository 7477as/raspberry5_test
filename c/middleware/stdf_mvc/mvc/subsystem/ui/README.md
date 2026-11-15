# 界面 subject 定义

> 声明按键、触摸及显示更新 subject，供界面业务扩展。

## 功能简介

UI 数据层预留按键、触摸和显示状态 ID。打印及 SDL2 界面位于 `app/ui/`，当前订阅 dm 温度；此目录不承担窗口和渲染逻辑。

## 依赖 / 环境

- 树莓派 5，Raspberry Pi OS Bookworm 64-bit，C11 和 pthread。
- 构建工具：`sudo apt install gcc cmake pkg-config`。
- 图形示例可选 `sudo apt install libsdl2-dev`，无模型资源。

## 构建与运行

随完整示例构建，从当前目录执行：

```bash
cd ../../..
./build_run.sh build
```

## 关键参数 / 配置

| 参数 / 宏 / 环境变量 | 默认值 | 说明 |
| :--- | :--- | :--- |
| UI subject ID | `300..302` | 按键、触摸、显示更新 |
| `STDF_MVC_UI_SUBJECT_LIST(X)` | 3 项 | UI subject 与名称的二元映射 |
| `STDF_MVC_UI_DATA_LIST(X)` | 空 | UI payload 注册 |
| `STDF_MVC_ENABLE_SDL2` | `ON` | 发现 SDL2 时构建图形业务 |

在 `stdf_mvc_subject_ui.h` 的 subject 清单追加 `X(id, "ui.name")`，用于生成枚举和 const 名称表。
在 `stdf_mvc_data_ui.h` 定义结构体并追加 `X(id, type)`，用于生成缓存和订阅表；两张表由上级聚合头分别汇总。

## 已知问题 / 注意事项

- 当前 UI ID 只作预留，不接受发布、读取和订阅。
- 回调由主循环 poll 执行，SDL2 事件与渲染也在主线程执行。
- 深度为 1 的覆盖语义不保证每个按键事件交付，扩展命令事件时应另行评估队列方案。

## 参考资料

- [完整构建与主循环派发](../../../README.md)
- [统一注册表](../stdf_mvc_subsystem.h)
