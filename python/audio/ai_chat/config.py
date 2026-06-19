# config.py
import os

# --- 硬件配置 ---
# 麦克风设备 ID (使用 'arecord -l' 查看，如 "hw:1,0" 或 "plughw:1,0")
MIC_DEVICE = "hw:0,0" 

# --- GStreamer 音频参数 ---
SAMPLE_RATE = 16000
CHANNELS = 1
DEBUG_WAV_PATH = "debug_recording.wav"

# --- Sherpa-ncnn 模型路径 ---
# 请修改为你实际解压的目录名
SHERPA_MODEL_DIR = "models/sherpa-ncnn-streaming-zipformer-zh-14M-2023-02-23"

# 自动拼接完整路径
TOKENS = os.path.join(SHERPA_MODEL_DIR, "tokens.txt")
ENCODER_PARAM = os.path.join(SHERPA_MODEL_DIR, "encoder_jit_trace-pnnx.ncnn.param")
ENCODER_BIN = os.path.join(SHERPA_MODEL_DIR, "encoder_jit_trace-pnnx.ncnn.bin")
DECODER_PARAM = os.path.join(SHERPA_MODEL_DIR, "decoder_jit_trace-pnnx.ncnn.param")
DECODER_BIN = os.path.join(SHERPA_MODEL_DIR, "decoder_jit_trace-pnnx.ncnn.bin")
JOINER_PARAM = os.path.join(SHERPA_MODEL_DIR, "joiner_jit_trace-pnnx.ncnn.param")
JOINER_BIN = os.path.join(SHERPA_MODEL_DIR, "joiner_jit_trace-pnnx.ncnn.bin")

# --- LLM 配置 ---
# 确保你已运行 'ollama pull qwen2:1.5b'
LLM_MODEL_NAME = "qwen2:1.5b"

# --- TTS 配置 ---
TTS_VOICE = "zh-CN-XiaoxiaoNeural"  # 微软 Edge-TTS 提供的优质中文女声
TTS_Rate = "+0%"                   # 语速
TTS_Volume = "+0%"                 # 音量
