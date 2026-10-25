# 双麦录音（GStreamer）

> 用 C++ + GStreamer 把两路麦克风录成双通道 WAV，可选额外拆出两个单通道文件，并实时打印每声道峰值 dBFS。

## 功能简介

这个示例解决的是**怎么在树莓派上把双麦硬件录成可分析的标准音频文件**。`alsasrc` 取到双通道原始数据，经 `audioconvert` / `audioresample` 统一成 S16LE，交给 `wavenc` 封装成 WAV 落盘。

与 `arecord` 的取舍：`arecord -c2 -r16000 -f S16_LE out.wav` 一行就能录，但换设备名、换采样率、拆分通道都要重新敲参数，也没有实时电平反馈。这个示例把管线写死在代码里，参数走命令行，同时每 500 ms 打印一次 MIC0/MIC1 的 peak / RMS dBFS，并统计该时间窗的满量程样本数。满量程样本持续出现说明数字采样已削波；仅峰值接近 `0 dBFS` 表示余量很小，不能单独证明波形已被截断。

与 `gst-launch-1.0` 的取舍：`gst-launch` 适合快速验证一条管道，但这个示例需要挂 probe 统计电平、需要在 Ctrl+C 时补发 EOS 保证 WAV 头完整，这些在命令行管道里做起来很别扭。

典型应用场景：录音链路调试、确认双麦两路都能出声、给后续 ASR / AEC 准备素材。

**输出文件**：

| 文件 | 内容 | 何时生成 |
| :--- | :--- | :--- |
| `output/capture.wav` | 双通道交织（ch0=MIC0，ch1=MIC1） | 总是 |
| `output/mic0.wav` | MIC0 单通道 | 加 `--split` |
| `output/mic1.wav` | MIC1 单通道 | 加 `--split` |

## 依赖 / 环境

- 硬件：树莓派 5 + 双麦声卡（WM8960 Audio HAT 或等价的双麦 USB / HAT 设备）
- 系统：Raspberry Pi OS Bookworm 64-bit
- 系统库：

```bash
sudo apt install cmake build-essential pkg-config \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
    gstreamer1.0-alsa gstreamer1.0-tools
```

- 音频权限：用户需要在 `audio` 组

```bash
sudo usermod -aG audio "$USER"
```

- 无模型 / 无额外资源依赖

## 构建与运行

```bash
cd cpp/audio/gstreamer/gst_capture
chmod +x build_run.sh
./build_run.sh                       # 默认：录到 Ctrl+C，输出 output/capture.wav
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
# 1. 录 10 秒
./build_run.sh --duration 10

# 2. 换设备（先查设备号）
./build_run.sh --device hw:1,0 --duration 10

# 3. 同时拆出两个单通道文件
./build_run.sh --split --duration 10

# 4. 换采样率（hw: 设备格式固定，建议改用 plughw:）
./build_run.sh --device plughw:0,0 --rate 48000 --duration 10

# 5. 指定输出路径
./build_run.sh --out /tmp/record.wav --duration 5
```

先确认设备号：

```bash
arecord -l   # 采集设备
aplay -l     # 播放设备
```

## 关键参数 / 配置

| 参数 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `--device <dev>` | `hw:0,0` | ALSA 采集设备，`hw:` 严格格式、`plughw:` 可转换 |
| `--rate <hz>` | `16000` | 采样率，`hw:` 设备只支持固定采样率 |
| `--duration <sec>` | `0` | 录制时长秒数，0 表示录到 Ctrl+C |
| `--out <path>` | `output/capture.wav` | 双通道 WAV 输出路径 |
| `--split` | 关 | 额外写出 `mic0.wav` / `mic1.wav` 单通道文件 |
| `-h`, `--help` | — | 显示用法 |

运行时每 500 ms 打印一次 `MIC0/MIC1 peak`、`rms` 和该时间窗中正好达到 S16 满量程的样本数。`full-scale` 大于 0 是数字削波的直接迹象；数值为 0 不排除模拟前级失真。

## 已知问题 / 注意事项

- **`hw:` 是严格格式设备**。`hw:0,0` 只接受硬件原生采样率与声道数，想换采样率要用 `plughw:0,0` 让 ALSA 做转换。
- **`wavenc` 靠 EOS 回填 WAV 头**。程序在 `Ctrl+C` 时会主动发 EOS 并等它走完，文件头才完整；直接 `kill -9` 会留下损坏的 WAV 头。
- **双麦必须保持 `channels=2`**。caps 里写 `channels=1` 会让 ALSA 下混，MIC1 直接丢失。
- **麦克风容易削顶**。满量程样本持续出现说明数字采样已削波，建议降低硬件采集增益并重新录制。峰值为 `0 dBFS` 只表示采样触及满量程；峰值留有余量也不能排除模拟前级在 ADC 前失真。
- **设备被占用会直接报错**。同一张卡上已有别的进程在录音时会报 `Device or resource busy`，先停掉占用者。
- 录音是实时的，录 10 秒就花 10 秒。

## 参考资料

- [GStreamer ALSA 插件文档](https://gstreamer.freedespan.org/documentation/alsa/index.html)
- [ALSA 设备命名规则（hw / plughw / default）](https://www.alsa-project.org/wiki/PCM_names)
- 同目录 `../gst_loopback`（麦克风回声回路）与 `../gst_aec_webrtcdsp`（webrtcdsp 回声消除）
