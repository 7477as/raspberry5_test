# 音频播放（GStreamer）

> 用 C++ + GStreamer 播放音频文件或生成测试音，输出可选 ALSA 直连或 PulseAudio（跟随 `wpctl` 默认设备）。

## 功能简介

这个示例解决的是**怎么在树莓派上把声音送到扬声器**。两种模式：给一个音频文件就播放它（`uridecodebin` 自动解码 wav / mp3 / flac / ogg），不给文件就放一段测试音（`audiotestsrc`），用来快速确认扬声器通路是否正常。

与 `gst-launch-1.0` 的取舍：`gst-launch-1.0 audiotestsrc ! alsasink` 适合快速验证，但文件播放要自己拼 `filesrc ! decodebin ! audioconvert ! alsasink`，遇到路径带空格就报错，而且换输出后端（ALSA / PulseAudio）得整条重写。这个示例把两条后端抽象成一个 `--sink` 参数，并把音量、`Ctrl+C` 停止、错误提示都做进代码里。

与 `aplay` / `paplay` 的取舍：`aplay` 只能放裸 PCM / WAV，不解 mp3 / flac，也不方便切到 PulseAudio 默认设备。这个示例走完整解码链，输出端可切。

与 `playbin` 的取舍：`playbin` 一把梭，但它的 `flags` 属性在 GStreamer 1.26 的公开头文件里没有导出枚举，想只放音频得硬编码魔数。这里改用 `uridecodebin` + 显式 `audioconvert` / `audioresample` / `volume`，链路看得见也改得动。

典型应用场景：确认新板子的扬声器能出声、调系统默认输出设备、给音频播放功能做冒烟测试。

## 依赖 / 环境

- 硬件：树莓派 5 + 声卡（HDMI 输出或 WM8960 Audio HAT）
- 系统：Raspberry Pi OS Bookworm 64-bit
- 系统库：

```bash
sudo apt install cmake build-essential pkg-config \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad gstreamer1.0-alsa gstreamer1.0-pulseaudio \
    gstreamer1.0-tools
```

- 音频权限：用户需要在 `audio` 组

```bash
sudo usermod -aG audio "$USER"
```

- 音频文件：任意格式（wav / mp3 / flac / ogg），用 `--file` 指定

## 构建与运行

```bash
cd cpp/audio/gstreamer/gst_playback
chmod +x build_run.sh
./build_run.sh                       # 默认：播放 440 Hz 测试音到 ALSA
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
# 1. 放一段测试音确认扬声器通路
./build_run.sh --tone 440

# 2. 播放指定文件
./build_run.sh --file <音频文件.mp3>

# 3. 走 PulseAudio，跟随 wpctl 设定的默认设备
./build_run.sh --file <音频文件.wav> --sink pulse

# 4. 指定 PulseAudio 的某个 sink
./build_run.sh --file <音频文件.wav> --sink pulse --device alsa_output.usb-Generic_USB_Audio-00

# 5. ALSA 直连指定设备并降低音量
./build_run.sh --file <音频文件.wav> --device plughw:0,0 --volume 0.3
```

先确认设备：

```bash
aplay -l     # ALSA 播放设备
wpctl status # PulseAudio 输出设备（ID 对应 --device 的名字）
```

## 关键参数 / 配置

| 参数 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `--file <path>` | 空 | 要播放的音频文件；给了它就不再放测试音 |
| `--tone <hz>` | `440` | 测试音频率（0 ~ 20000），`--file` 存在时忽略 |
| `--sink alsa\|pulse` | `alsa` | 输出后端 |
| `--device <dev>` | `hw:0,0` | ALSA 设备；`--sink pulse` 时是 Pulse sink 名，留空用默认设备 |
| `--volume <0.0-1.0>` | `1.0` | 音量 |
| `-h`, `--help` | — | 显示用法 |

## 已知问题 / 注意事项

- **ALSA 的 `hw:` 是严格格式设备**。文件采样率跟硬件原生不一致时会协商失败，换成 `plughw:0,0` 让 ALSA 做转换。`--device plughw:0,0` 是最省心的选择。
- **`hw:0,0` 在本机可能指的是 HDMI**。用 `aplay -l` 确认；插了 HAT 时设备号通常会变。
- **`--sink pulse` 依赖 PulseAudio / PipeWire 正在运行**。没跑起来会报连接失败，先 `pactl info` 确认。`wpctl set-default <ID>` 设的默认设备就是 `--sink pulse` 不带 `--device` 时用的那个。
- **设备被占用会直接失败**。浏览器、其它 GStreamer 程序占着声卡时先停掉，程序会提示 `Device or resource busy`。
- **测试音是 `is-live` 的**，不会自己结束，要 `Ctrl+C` 停。播放文件则会自己播完退出。
- 文件路径带空格或中文没问题，内部用 `gst_filename_to_uri` 转义。

## 参考资料

- [GStreamer ALSA 插件文档](https://gstreamer.freedespan.org/documentation/alsa/index.html)
- [PulseAudio 设备管理（wpctl）](https://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/User/Commands/)
- 同目录 `../gst_capture`（双麦录音）与 `../gst_loopback`（麦克风回声回路）
