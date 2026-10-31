# PipeWire 播放基础示例

> 使用 PipeWire 播放 440 Hz 正弦测试音，验证默认扬声器播放链路。

## 功能简介

这个示例直接使用 PipeWire `pw_stream` 创建播放流，在回调中生成交错的 `S16LE` PCM 正弦波并连接到默认输出设备。它比 GStreamer 示例更接近 PipeWire 的基础客户端流程，同时避免在代码中硬编码 ALSA `hw:0,0` 设备名。

与 ALSA 直接播放相比，PipeWire 通过会话管理器选择默认设备，并负责把应用流接入当前音频图；与 GStreamer 相比，它不包含媒体文件解码和复杂管线。典型场景是验证 PipeWire 服务、默认输出设备和应用侧 PCM 播放代码。

## 依赖 / 环境

- 硬件：树莓派 5 + WM8960 Audio HAT 或其它 PipeWire 输出设备
- 系统：Raspberry Pi OS Bookworm 64-bit
- 系统库：`sudo apt install cmake build-essential pkg-config libpipewire-0.3-dev`
- 音频服务：PipeWire 与会话管理器正常运行

## 构建与运行

```bash
cd cpp/audio/pipeware/pipewire_playback
chmod +x build_run.sh
./build_run.sh
```

```bash
./build_run.sh build     # 仅构建
./build_run.sh clean     # 清理 build 目录
./build_run.sh rebuild   # 清理后重新构建并运行
NORUN=1 ./build_run.sh   # 仅构建不运行
BUILD_TYPE=Debug ./build_run.sh
```

运行前查看 PipeWire 默认设备：

```bash
wpctl status
```

指定测试参数：

```bash
./build_run.sh --tone 1000 --volume 0.1 --duration 5
```

## 关键参数 / 配置

| 参数 / 宏 / 环境变量 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `--rate` | `48000` | 播放采样率 |
| `--channels` | `2` | 播放声道数 |
| `--tone` | `440` | 正弦测试音频率，单位 Hz |
| `--volume` | `0.2` | 音量，范围 0.0 到 1.0 |
| `--duration` | `0` | 播放秒数，0 表示持续到 Ctrl+C |
| `BUILD_TYPE` | `Release` | CMake 构建类型 |
| `NORUN` | `0` | 设为 `1` 时只构建不运行 |

## 已知问题 / 注意事项

- 必须先确认 PipeWire 服务和会话管理器正在运行；`wpctl status` 可查看默认输出设备。
- 默认输出由 PipeWire 会话管理器选择，程序不提供 ALSA 设备号参数。
- 音量默认较低，但首次测试仍建议先降低功放音量。
- `--rate` 和 `--channels` 需要得到当前 PipeWire 音频图的格式协商支持。

## 参考资料

- [PipeWire 官方文档](https://docs.pipewire.org/)
- [PipeWire Stream API](https://docs.pipewire.org/page_main.html)
- [wpctl 文档](https://pipewire.pages.freedesktop.org/wireplumber/tools/wpctl.html)
