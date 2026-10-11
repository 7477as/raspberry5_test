# WebRTC DSP 实时回声消除（双麦四通道录音）

> 用 C++ + GStreamer C API 做硬件回环回声消除：参考音频经扬声器播放制造真实回声，WM8960 HAT 采集双麦，MIC0 送 `webrtcdsp` 处理，结果存成 MIC0 / MIC1 / 参考 / 算法后 的四通道 WAV。

## 功能简介

这个示例解决的是**怎么在树莓派上用双麦做硬件回环 AEC 并留下可分析的录音**。参考音频从扬声器放出来，声音经过房间反射被两路麦克风收回，MIC0 送进 `webrtcdsp` 做回声消除、消噪、AGC、高通，最后四路信号交织成一个四通道 WAV。

与 `gst-launch-1.0` 的取舍：`gst-launch` 适合快速验证，但下混、通道顺序和对齐都得手工拼，出错时也不好定位。这个示例把管线写死在代码里，固定输出四通道 WAV，并打印算法延迟与 ERLE，便于反复调参和对比。

与离线合成回声的取舍：离线合成能精确控制回声延迟和幅度、ERLE 可复现，但测不出真实声学路径（房间反射、扬声器失真、双麦相位差）。本示例走真实硬件回环，更接近产品现场，代价是每次结果受房间和音量影响。

典型应用场景：智能音箱、语音面板、会议设备做 AEC 参数调试时，录一份四通道素材交给算法同事分析。

**输出文件**（默认 `./output/aec_4ch.wav`）：

| 通道 | 含义 | 是否进算法 |
| :--- | :--- | :--- |
| ch0 | MIC0（麦克风 0） | 是 |
| ch1 | MIC1（麦克风 1） | 否 |
| ch2 | 参考（扬声器播放的信号） | 作为 AEC 参考 |
| ch3 | 算法后（`webrtcdsp` 输出，默认已按算法延迟对齐） | — |

## 依赖 / 环境

- 硬件：树莓派 5 + WM8960 Audio HAT（双麦输入 + 扬声器输出）
- 系统：Raspberry Pi OS Bookworm 64-bit
- 系统库：

```bash
sudo apt install cmake build-essential pkg-config \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-good gstreamer1.0-plugins-bad \
    gstreamer1.0-alsa gstreamer1.0-tools
```

- 音频权限：用户需要在 `audio` 组

```bash
sudo usermod -aG audio "$USER"
```

- 无模型 / 无额外资源依赖，测试音频可任意指定

## 构建与运行

```bash
cd cpp/audio/aec_webrtcdsp
chmod +x build_run.sh
./build_run.sh                       # 默认：按参考音频全长实时录制
```

常用子命令与开关：

```bash
./build_run.sh build     # 仅构建
./build_run.sh clean     # 清理 build 目录
./build_run.sh rebuild   # 清理后重新构建
NORUN=1 ./build_run.sh   # 仅构建不运行
BUILD_TYPE=Debug ./build_run.sh
```

典型用法：

```bash
# 1. 录 10 秒（推荐先从短时长开始确认接线）
./build_run.sh --duration 10

# 2. 指定参考音频与时长
./build_run.sh --ref <参考音频.wav> --duration 20

# 3. 指定 ALSA 设备
./build_run.sh --playback-device hw:0,0 --capture-device hw:0,0 --duration 15

# 4. 关闭回声消除做对照组
./build_run.sh --duration 10 --no-echo-cancel --no-noise-suppression --no-gain-control

# 5. 录长一点，用 Ctrl+C 提前结束（已采集的数据照常保存）
./build_run.sh --ref <参考音频.wav>
```

先确认设备号：

```bash
aplay -l    # 播放设备，看 wm8960soundcard 是第几张卡
arecord -l  # 采集设备
```

## 关键参数 / 配置

| 参数 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `--ref <path>` | `$HOME/work/audio/testAudio/test.wav` | 参考音频，会被播放出去制造回声 |
| `--playback-device <dev>` | `hw:0,0` | ALSA 播放设备（扬声器） |
| `--capture-device <dev>` | `hw:0,0` | ALSA 采集设备（双麦） |
| `--duration <sec>` | `0`（自动） | 录制时长秒数，0 表示按参考音频全长；播完 EOS 也会提前收尾 |
| `--rate <hz>` | `16000` | 采样率，仅支持 8000/16000/32000/48000 |
| `--no-echo-cancel` | 关 | 关闭回声消除 |
| `--no-noise-suppression` | 关 | 关闭降噪 |
| `--no-gain-control` | 关 | 关闭自动增益 |
| `--no-high-pass` | 关 | 关闭高通滤波 |
| `--out-dir <dir>` | `output` | 输出目录 |
| `--no-align` | 关 | 不按算法延迟对齐 ch3 |

终端会打印各通道峰值、算法延迟（ms）、处理前后 RMS 和 ERLE（dB）。

## 已知问题 / 注意事项

- **`webrtcdsp` 只处理通道 0**。实测喂双声道交错信号时，ch0 会被算法改动，ch1 原样通过。所以 MIC0 是送进算法的那一路，MIC1 只做录音留档。
- **mic 侧 caps 必须是 `channels=2`**。写成 `channels=1` 会把双麦下混，MIC1 直接丢失，这是最容易踩的坑。WM8960 的 `asound.conf` 里 `pcm.array` 是 `dsnoop` + `channels 2`，硬件确实输出两路。
- **麦克风容易削顶**。实测 MIC0/MIC1 峰值打满 1.0，约 1% 采样点接近满量程。削顶会明显拉低 AEC 效果，建议先调低 ALSA 麦克风增益或扬声器音量，让峰值落在 0.7 左右。
- 录制是**实时**的，耗时等于音频时长；20 秒音频就要录 20 秒。中途 `Ctrl+C` 会正常收尾并保存已采集数据。
- 参考通道录的是**送进扬声器之前**的信号，不是房间里的实际声场。
- ERLE 反映的是 MIC0 与算法后输出的整体能量比。若扬声器音量太低、麦克风离得太远，参考与麦克风的互相关会很低，ERLE 可能主要来自降噪而不是回声消除——对照运行 `--no-echo-cancel --no-noise-suppression --no-gain-control` 即可区分。
- **拆通道不要用 `channelsplit`**。本程序写出的 WAV 没有声道掩码，`channel_layout=quad` 会触发 ffmpeg 重新映射导致内容不对。用 `pan` 滤镜逐路抽：

```bash
# 逐路抽出 ch0 ~ ch3（MIC0 / MIC1 / 参考 / 算法后）
for i in 0 1 2 3; do
    ffmpeg -y -v error -i output/aec_4ch.wav -af "pan=mono|c0=c$i" "/tmp/ch${i}.wav"
done

# 确认确实是四通道
ffprobe -v error -show_entries stream=channels,sample_rate,bits_per_sample \
    -of default=noprint_wrappers=1 output/aec_4ch.wav
```

## 参考资料

- [GStreamer webrtcdsp 插件文档](https://gstreamer.freedesktop.org/documentation/webrtcdsp/webrtcdsp.html)
- [WebRTC Audio Processing Library](https://webrtc.googlesource.com/src/#/modules/audio_processing)
- [WM8960 Audio HAT 说明](https://github.com/Waveshare/WM8960-Audio-HAT)
- 相关 API：`gst_parse_launch`、`gst_pad_add_probe`、`gst_element_query_duration`、`g_unix_signal_add`