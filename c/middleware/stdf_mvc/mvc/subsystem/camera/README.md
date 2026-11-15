# 摄像头 subject 定义

> 声明摄像头与 AI 结果的 subject，为摄像头业务提供数据契约。

## 功能简介

预留原始帧、编码帧、AI 结果和云台状态 ID，供应用间交换状态。此目录仅定义契约，采集由 `app/camera/` 扩展；相较直接传递视频帧，最新值模型更适合结果和状态更新。

## 依赖 / 环境

- 树莓派 5，Raspberry Pi OS Bookworm 64-bit，C11 和 pthread。
- 构建工具：`sudo apt install gcc cmake pkg-config`。
- 当前无实际采集，不需要摄像头、模型或图像库。

## 构建与运行

随完整示例构建，从当前目录执行：

```bash
cd ../../..
./build_run.sh build
```

## 关键参数 / 配置

| 参数 / 宏 / 环境变量 | 默认值 | 说明 |
| :--- | :--- | :--- |
| 摄像头 subject ID | `200..203` | 原始帧、编码帧、AI 结果、云台 |
| `STDF_MVC_CAMERA_SUBJECT_LIST(X)` | 4 项 | 摄像头 subject 与名称的二元映射 |
| `STDF_MVC_CAMERA_DATA_LIST(X)` | 空 | 摄像头 payload 注册 |

在 `stdf_mvc_subject_camera.h` 的 subject 清单追加 `X(id, "camera.name")`，用于生成枚举和 const 名称表。
在 `stdf_mvc_data_camera.h` 定义结构体并追加 `X(id, type)`，用于生成缓存和订阅表；两张表由上级聚合头分别汇总。

## 已知问题 / 注意事项

- 当前摄像头 ID 只作预留，不接受发布、读取和订阅。
- 缓存复制结构体不会延长结构体内指针指向资源的生命周期，帧资源须单独管理。

## 参考资料

- [完整构建与主循环派发](../../../README.md)
- [统一注册表](../stdf_mvc_subsystem.h)
