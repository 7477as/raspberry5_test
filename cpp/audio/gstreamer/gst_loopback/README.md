# 麦克风回声回路（GStreamer）

> 用 C++ + GStreamer 把双麦采集的声音直接送回扬声器，可选只送 MIC0 或只送 MIC1。

## 功能简介

这个示例解决的是**怎么验证双麦声卡的采集与播放链路是通的**。麦克风的声音被实时送进扬声器，用耳朵就能听出哪一路麦在响、增益够不够、有没有啸叫。

与 `gst-launch-1.0` 的取舍：`gst-launch-1.0 alsasrc device=hw:0,0 ! audioconvert ! audioresample ! alsasink device=hw:0,0` 就能做直通，但一旦要「只放左麦」就得写 `deinterleave` / `interleave` 的分支拓扑，命令又长又容易写错（见下方注意事项）。这个示例把拓扑固定下来，只用一个 `--mode` 切换路由。

与 `alsaloop` 的取舍：`alsaloop` 只能整路直通，没法单独选麦克风；这个示例能按通道路由。

典型应用场景：接上 HAT 后确认两路麦都能用、调麦克风增益、验证扬声器、给回声消除做前置自检。

## 依赖 / 环境

- 硬件：树莓派 5 + WM8960 Audio HAT（双麦输入 + 扬声器输出，同一张卡）
- 系统：Raspberry Pi OS Bookworm 64-bit
- 系统库：

```bash
sudo apt install cmake build-essential pkg-config \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-base gstreamer1.0-alsa gstreamer1.0-tools
```

- 音频权限：用户需要在 `audio` 组

```bash
sudo usermod -aG audio "$USER"
```

- 无模型 / 无额外资源依赖

## 构建与运行

```bash
cd cpp/audio/gstreamer/gst_loopback
chmod +x build_run.sh
./build_run.sh                       # 默认：双麦直通回环，跑到 Ctrl+C 结束
```

子命令与开关：

```bash
./build_run.sh build     # 仅构建
./build_run.sh clean     # 清理 build 目录
./build_run.sh rebuild   # 清理后重新构建
NORUN=1 ./build_run.sh   # 仅构建不运行
BUILD_TYPE=Debug ./build_run.sh
```

典型用法：

```bash
# 1. 双麦回环，确认两路都在响
./build_run.sh

# 2. 只回放 MIC0
./build_run.sh --mode left

# 3. 只回放 MIC1
./build_run.sh --mode right

# 4. 换设备（先查设备号）
./build_run.sh --device hw:1,0

# 5. 跑 10 秒自动停
./build_run.sh --duration 10
```

先确认设备号：

```bash
arecord -l   # 采集设备
aplay -l     # 播放设备
```

## 关键参数 / 配置

| 参数 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `--mode both\|left\|right` | `both` | 送回哪一路：`both` = MIC0+MIC1，`left` = 仅 MIC0，`right` = 仅 MIC1 |
| `--device <dev>` | `hw:0,0` | ALSA 设备，采集与播放共用同一个 |
| `--duration <sec>` | `0` | 运行时长秒数，0 表示跑到 Ctrl+C |
| `-h`, `--help` | — | 显示用法 |

## 已知问题 / 注意事项

- 双麦模式直接按 ALSA 设备协商格式回环。单麦模式通过 `deinterleave` 选中声道，再由 `interleave` 输出到声卡；此拓扑按该 WM8960 设备的可运行命令验证。

```bash
# 双麦直通
 gst-launch-1.0 alsasrc device=hw:0,0 ! audioconvert ! audioresample ! \
    alsasink device=hw:0,0

# 只回放 MIC0
 gst-launch-1.0 alsasrc device=hw:0,0 ! audioconvert ! \
    deinterleave name=d interleave name=i ! audioconvert ! \
    alsasink device=hw:0,0 d.src_0 ! queue ! i.sink_0

# 只回放 MIC1
 gst-launch-1.0 alsasrc device=hw:0,0 ! audioconvert ! \
    deinterleave name=d interleave name=i ! audioconvert ! \
    alsasink device=hw:0,0 d.src_1 ! queue ! i.sink_1
```

- **采集和播放用同一个设备号**。WM8960 HAT 是全双工的，同一个 `hw:0,0` 既能收也能放。
- **啸叫是正常的**。扬声器的声音被麦克风收回去再放大就是自激，音量调小或把麦克风拿远一点。要真正消除回声请用同目录的 `../gst_aec_webrtcdsp`（webrtcdsp AEC）。
- **设备被占用或 ALSA 格式不支持**会导致管线启动失败。用 `fuser -v /dev/snd/*` 查看占用进程。
- 运行时出现 `Internal data stream error` 时，程序会报告出错元素和 GStreamer 调试信息。仍需更多日志时运行 `GST_DEBUG=2 ./build_run.sh`。
- 这类流错误通常是设备打开或格式协商/I/O 失败，不是麦克风增益过高直接导致。增益过高会让音频削顶；建议按 `amixer` 输出降低 `Capture` 与输入 Boost。

## 参考资料

- [GStreamer ALSA 插件文档](https://gstreamer.freedespan.org/documentation/alsa/index.html)
- [GStreamer `deinterleave` / `interleave` 元素文档](https://gstreamer.freedespan.org/documentation/gst-plugins-base/index.html)
- 同目录 `../gst_capture`（双麦录音）、`../gst_playback`（音频播放）、`../gst_aec_webrtcdsp`（webrtcdsp 回声消除）
