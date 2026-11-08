# AI 语音助手

> 树莓派 5 上的中文语音对话示例：麦克风收音 → Sherpa 流式识别 → Qwen2 回答 → EdgeTTS 播报。

## 功能简介

本示例解决“无屏幕场景下和树莓派对话”的问题：不用 rpicam / 桌面录音机，直接用 GStreamer 从 USB 声卡取 16k 单声道 PCM，送给 Sherpa-ncnn 做流式中文识别，端点检测后调本地 Ollama（Qwen2）生成回复，最后用 EdgeTTS + mpv 播出来。典型场景是桌面语音助手、口语问答 demo。

## 依赖 / 环境

- 硬件：树莓派 5 + USB 麦克风 + USB/3.5mm 音箱（麦克风和扬声器可以是两张声卡）
- 系统：Raspberry Pi OS Bookworm 64-bit
- 系统库：`sudo apt install python3-gi gir1.2-gstreamer-1.0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-alsa alsa-utils mpv ollama`
- Python 包：见 `requirements.txt`
- 模型 / 资源：`models/sherpa-ncnn-streaming-zipformer-zh-14M-2023-02-23/` 已随仓库附带；LLM 需 `ollama pull qwen2:1.5b`

## 构建与运行

### Python 示例

```bash
cd python/audio/ai_chat
chmod +x run.sh
./run.sh list    # 先看声卡，记下麦克风/扬声器编号
./run.sh         # 启动语音助手
```

首次运行前，先激活你的虚拟环境并安装依赖：

```bash
source /home/moyuping/myenv/bin/activate
pip install -r requirements.txt
```

其它子命令：

```bash
./run.sh clean   # 删除调试录音与 __pycache__
./run.sh -h      # 查看帮助
```

## 关键参数 / 配置

`config.py` 中麦克风与扬声器完全分开，也可用环境变量覆盖：

| 参数 / 宏 / 环境变量 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `MIC_DEVICE` / `AI_CHAT_MIC` | `plughw:1,0` | 麦克风 ALSA 设备，对应 `arecord -l` |
| `SPEAKER_DEVICE` / `AI_CHAT_SPK` | 空（系统默认） | 扬声器 ALSA 设备，如 `plughw:0,0`；为空走 mpv 默认输出 |
| `SAMPLE_RATE` | `16000` | 识别采样率 |
| `CHANNELS` | `1` | 单声道 |
| `DEBUG_WAV_PATH` | `./debug_recording.wav` | 调试录音，置空可关闭 |
| `LLM_MODEL_NAME` / `AI_CHAT_LLM` | `qwen2:1.5b` | Ollama 模型名 |
| `TTS_VOICE` | `zh-CN-XiaoxiaoNeural` | EdgeTTS 中文女声 |
| `TTS_Rate` / `TTS_Volume` | `+0%` | 语速 / 音量 |

临时换声卡不用改文件：

```bash
AI_CHAT_MIC=plughw:2,0 AI_CHAT_SPK=plughw:0,0 ./run.sh
```

## 已知问题 / 注意事项

- 麦克风选 `hw:x,y` 容易因采样率不支持报错，优先用 `plughw:x,y` 自动重采样
- 说话时 TTS 播放会被暂停录音，避免回音；播完会自动恢复收音
- EdgeTTS 需要联网；Ollama 首次拉模型较慢，建议提前 `ollama pull qwen2:1.5b`
- 录音全 0 一般是 ALSA 设备被占用，用 `./run.sh list` 确认编号后重试

## 参考资料

- [sherpa-ncnn 官方文档](https://github.com/k2-fsa/sherpa-ncnn)
- [EdgeTTS 项目](https://github.com/rany2/edge-tts)
- [Ollama 文档](https://ollama.com/)
