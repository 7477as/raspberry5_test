"""AI 语音助手配置：麦克风与扬声器可分开指定。"""
from __future__ import annotations

import os
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent

# --- 麦克风（录音）---
# 用 `./run.sh list` 查看 arecord -l，填 card/device，如 plughw:1,0
MIC_DEVICE = os.environ.get("AI_CHAT_MIC", "plughw:2,0")

# --- 扬声器（播放）---
# 两种写法二选一：
# 1) SPEAKER_DEVICE 为空：mpv 走系统默认输出
# 2) 填 ALSA 名如 plughw:0,0：mpv 强制走该声卡
SPEAKER_DEVICE = os.environ.get("AI_CHAT_SPK", "plughw:3,0")

# --- GStreamer 音频参数 ---
SAMPLE_RATE = 16000
CHANNELS = 1
DEBUG_WAV_PATH = str(BASE_DIR / "debug_recording.wav")

# --- Sherpa-ncnn 模型路径 ---
SHERPA_MODEL_DIR = str(BASE_DIR / "models/sherpa-ncnn-streaming-zipformer-zh-14M-2023-02-23")

TOKENS = os.path.join(SHERPA_MODEL_DIR, "tokens.txt")
ENCODER_PARAM = os.path.join(SHERPA_MODEL_DIR, "encoder_jit_trace-pnnx.ncnn.param")
ENCODER_BIN = os.path.join(SHERPA_MODEL_DIR, "encoder_jit_trace-pnnx.ncnn.bin")
DECODER_PARAM = os.path.join(SHERPA_MODEL_DIR, "decoder_jit_trace-pnnx.ncnn.param")
DECODER_BIN = os.path.join(SHERPA_MODEL_DIR, "decoder_jit_trace-pnnx.ncnn.bin")
JOINER_PARAM = os.path.join(SHERPA_MODEL_DIR, "joiner_jit_trace-pnnx.ncnn.param")
JOINER_BIN = os.path.join(SHERPA_MODEL_DIR, "joiner_jit_trace-pnnx.ncnn.bin")

# --- LLM 配置 ---
LLM_MODEL_NAME = os.environ.get("AI_CHAT_LLM", "qwen2:1.5b")

# --- TTS 配置 ---
TTS_VOICE = "zh-CN-XiaoxiaoNeural"
TTS_Rate = "+0%"
TTS_Volume = "+0%"


def mpv_audio_args() -> list[str]:
    """根据 SPEAKER_DEVICE 生成 mpv 的音频输出参数。"""
    if not SPEAKER_DEVICE:
        return []
    device = SPEAKER_DEVICE
    if device.startswith("plughw:") or device.startswith("hw:"):
        device = f"alsa/{device}"
    return [f"--audio-device={device}"]
