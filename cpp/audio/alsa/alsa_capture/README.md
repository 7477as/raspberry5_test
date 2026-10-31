# ALSA 采集基础示例

> 使用 ALSA 采集 PCM 音频并保存为 WAV 文件，验证麦克风采集链路。

## 功能简介

这个示例直接打开 ALSA capture 设备，配置为交错的 `S16_LE` PCM 数据，按固定时长读取音频帧并写入标准 WAV 文件。它比 GStreamer 示例更接近 ALSA 的最小采集流程，适合确认设备号、采样率、声道数和麦克风是否正常。

与 `arecord` 相比，本示例把 ALSA 打开、读取和 WAV 头写入过程放在 C++ 源码中；与 GStreamer 相比，它不负责管线、格式转换或编码。典型场景是麦克风驱动调试、采集链路自检和后续音频算法的输入准备。

## 依赖 / 环境

- 硬件：树莓派 5 + WM8960 Audio HAT 或其它 ALSA 采集设备
- 系统：Raspberry Pi OS Bookworm 64-bit
- 系统库：

```bash
sudo apt install cmake build-essential pkg-config libasound2-dev
```

- 音频权限：用户需要在 `audio` 组

```bash
sudo usermod -aG audio "$USER"
```

## 构建与运行

```bash
cd cpp/audio/alsa/alsa_capture
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

先查看采集设备：

```bash
arecord -l
```

指定 WM8960 设备并保存到目标文件：

```bash
./build_run.sh --device plughw:CARD=wm8960soundcard,DEV=0 --output <输出.wav> --rate 48000 --channels 2 --duration 5
```

## 关键参数 / 配置

| 参数 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `--device <dev>` | `plughw:CARD=wm8960soundcard,DEV=0` | ALSA 采集设备；使用 WM8960 卡名，避免依赖动态卡号 |
| `--output <path>` | `capture.wav` | WAV 输出路径 |
| `--rate <hz>` | `48000` | 采样率 |
| `--channels <count>` | `2` | 声道数 |
| `--duration <sec>` | `5` | 采集时长 |

## 已知问题 / 注意事项

- 采集过程中如果出现 `Input/output error`，程序会先调用 ALSA 的恢复接口重置 PCM 状态；连续恢复失败通常表示设备参数或驱动链路仍不可用。
- `hw:` 要求硬件支持指定格式；遇到格式不支持时，使用 `--device plughw:0,0` 让 ALSA 做转换。
- 可以先用 ALSA 命令行工具验证同一设备和格式：

```bash
arecord -l
arecord -D <设备> -f S16_LE -r 48000 -c 2 -d 5 test.wav
```

- 设备被其它程序占用时会打开失败，可用 `fuser -v /dev/snd/*` 检查。
- 采集结束后才会回写 WAV 文件头；程序被强制终止时，输出文件可能无法播放。
- 默认输出文件位于执行目录；运行前确认有写权限。

## 参考资料

- [ALSA Library API](https://www.alsa-project.org/wiki/ALSA_Library_API)
- [ALSA PCM Interface](https://www.alsa-project.org/wiki/PCM)
- 同级 `../alsa_playback`：ALSA 播放正弦测试音示例
