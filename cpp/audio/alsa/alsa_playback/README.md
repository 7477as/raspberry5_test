# ALSA 播放基础示例

> 使用 ALSA 播放 440 Hz 正弦测试音，验证扬声器播放链路。

## 功能简介

这个示例直接打开 ALSA playback 设备，配置为交错的 `S16_LE` PCM 数据，生成固定频率的正弦波并写入声卡。它比 GStreamer 示例更接近 ALSA 的最小播放流程，适合确认设备号、采样率、声道数和扬声器是否正常。

与 `aplay` 相比，本示例不依赖输入音频文件；与 GStreamer 相比，它不负责解码、转换或自动管线协商。典型场景是声卡驱动调试、硬件出声自检和后续 ALSA 播放代码的起点。

## 依赖 / 环境

- 硬件：树莓派 5 + WM8960 Audio HAT 或其它 ALSA 播放设备
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
cd cpp/audio/alsa/alsa_playback
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

先查看播放设备：

```bash
aplay -l
```

指定设备和参数：

```bash
./build_run.sh --device <ALSA播放设备> --rate 48000 --channels 2 --tone 440 --duration 5
```

## 关键参数 / 配置

| 参数 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `--device <dev>` | `hw:0,0` | ALSA 播放设备；格式不匹配时可尝试 `plughw:0,0` |
| `--rate <hz>` | `48000` | 采样率 |
| `--channels <count>` | `2` | 声道数 |
| `--tone <hz>` | `440` | 正弦测试音频率 |
| `--duration <sec>` | `5` | 播放时长 |

## 已知问题 / 注意事项

- `hw:0,0` 只是默认示例，必须用 `aplay -l` 确认实际设备号。
- `hw:` 要求硬件支持指定格式；遇到格式不支持时，使用 `--device plughw:0,0` 让 ALSA 做转换。
- 设备被其它程序占用时会打开失败，可用 `fuser -v /dev/snd/*` 检查。
- 播放测试音时请先降低功放音量，避免突然产生较大声音。

## 参考资料

- [ALSA Library API](https://www.alsa-project.org/wiki/ALSA_Library_API)
- [ALSA PCM Interface](https://www.alsa-project.org/wiki/PCM)
- 同级 `../alsa_capture`：ALSA 采集并保存 WAV 示例
