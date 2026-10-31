# PipeWire 采集基础示例

> 使用 PipeWire 采集默认输入设备的 PCM 音频并保存为 WAV 文件。

## 功能简介

这个示例直接使用 PipeWire `pw_stream` 创建采集流，从默认输入设备读取交错的 `S16LE` PCM 数据，并写入标准 RIFF/WAV 文件。它保留了最少的音频处理逻辑，适合确认 PipeWire 默认输入设备、采样格式和应用侧采集回调是否正常。

与 ALSA 直接采集相比，PipeWire 通过会话管理器选择默认设备，并把应用流接入当前音频图；与 GStreamer 相比，它不负责编码、解码或复杂管线。典型场景是麦克风链路自检、录音程序原型和后续音频算法的输入准备。

## 依赖 / 环境

- 硬件：树莓派 5 + WM8960 Audio HAT、USB 麦克风或其它 PipeWire 输入设备
- 系统：Raspberry Pi OS Bookworm 64-bit
- 系统库：`sudo apt install cmake build-essential pkg-config libpipewire-0.3-dev`
- 音频服务：PipeWire 与会话管理器正常运行

## 构建与运行

```bash
cd cpp/audio/pipeware/pipewire_capture
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

录制指定时长并保存到目标路径：

```bash
./build_run.sh run --duration 5 --out <输出目录>/capture.wav
```

指定 PipeWire source 时，先通过 `wpctl status` 查看输入节点，再将节点名称传入：

```bash
./build_run.sh --source <SOURCE_NODE_NAME> --duration 5 --out capture.wav
```

## 关键参数 / 配置

| 参数 / 宏 / 环境变量 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `--rate` | `48000` | 采集采样率 |
| `--channels` | `2` | 采集声道数 |
| `--duration` | `0` | 采集秒数，0 表示持续到 Ctrl+C |
| `--out` | `capture.wav` | WAV 输出路径 |
| `--source` | 系统默认 | 指定 PipeWire source 节点名（`node.name`） |
| `BUILD_TYPE` | `Release` | CMake 构建类型 |
| `NORUN` | `0` | 设为 `1` 时只构建不运行 |

## 已知问题 / 注意事项

- 必须先确认 PipeWire 服务和会话管理器正在运行；`wpctl status` 可查看默认输入设备。
- 程序启动后应看到“采集流已连接并开始接收音频数据”，运行时每秒输出一次 PCM 峰值；若峰值持续为 `0`，检查麦克风硬件增益、Capture 音量和输入路由。
- 默认输入由 PipeWire 会话管理器选择；若默认 source 静音或不是麦克风，可用 `--source <SOURCE_NODE_NAME>` 指定实际输入节点。
- 程序正常退出或按 Ctrl+C 后才会回填 WAV 文件头；强制终止可能留下无法播放的文件。
- 基础 RIFF/WAV 头使用 32 位数据长度，单文件不能超过 4 GiB。
- `--rate` 和 `--channels` 需要得到当前 PipeWire 音频图的格式协商支持。

## 参考资料

- [PipeWire 官方文档](https://docs.pipewire.org/)
- [PipeWire Stream API](https://docs.pipewire.org/page_main.html)
- [wpctl 文档](https://pipewire.pages.freedesktop.org/wireplumber/tools/wpctl.html)
