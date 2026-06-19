import numpy as np
from openwakeword.model import Model
import os
import time
import subprocess
import sys

# 屏蔽底层警告
os.environ['ORT_LOGGING_LEVEL'] = '3'

# 核心参数
WAKEWORD_NAME = "alexa"              
THRESHOLD = 0.5                      
COOLDOWN_SECONDS = 2                 
CHUNK = 1280                         
RATE = 16000                         
BYTES_PER_READ = CHUNK * 2           # 16-bit 音频每个采样点 2 字节

def read_exact(pipe, size):
    """确保从管道中精准读取指定字节数的数据"""
    data = b''
    while len(data) < size:
        packet = pipe.read(size - len(data))
        if not packet:
            break
        data += packet
    return data

def run_wakeword_engine():
    print("\n[系统信息] 正在从缓存加载 ONNX 引擎...")
    try:
        # 加载 openwakeword 模型
        oww_model = Model(
            wakeword_models=[WAKEWORD_NAME], 
            inference_framework="onnx"
        )
    except Exception as e:
        print(f"❌ 模型加载失败: {e}")
        return

    # 🌟 GStreamer 魔法管道 🌟
    gst_cmd = [
        'gst-launch-1.0', '-q',
        'alsasrc', 'device=hw:0,0',
        '!', 'deinterleave', 'name=d',  
        'd.src_0', '!', 'queue',        
        '!', 'audioconvert',
        '!', 'audioresample',
        '!', f'audio/x-raw,format=S16LE,channels=1,rate={RATE}',
        '!', 'fdsink', 'fd=1', 'sync=false'
    ]

    print("🚀 启动底层 GStreamer 硬件麦克风...")
    process = subprocess.Popen(gst_cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)

    print("\n" + "="*40)
    print(f"🎤 监听中... 请尝试说: '{WAKEWORD_NAME.upper()}'")
    print("="*40 + "\n")

    last_trigger_time = 0  

    try:
        while True:
            # 1. 从 C 语言级别的管道精准抓取 2560 字节的完美音频
            audio_bytes = read_exact(process.stdout, BYTES_PER_READ)
            if not audio_bytes or len(audio_bytes) < BYTES_PER_READ:
                print("\n⚠️ 音频流中断。")
                break

            # 2. 转换为 int16 的 numpy 数组 (这是 openwakeword 唯一需要的格式)
            audio_np = np.frombuffer(audio_bytes, dtype=np.int16)

            # 3. 将数组送入引擎进行推理
            oww_model.predict(audio_np)

            # 4. 解析结果
            for mdl in oww_model.models:
                score = oww_model.prediction_buffer[mdl][-1]
                current_time = time.time()

                if score > THRESHOLD and (current_time - last_trigger_time) > COOLDOWN_SECONDS:
                    print(f"🌟 [检测到唤醒词]: {mdl.upper()} | 置信度: {score:.2f}")
                    last_trigger_time = current_time

    except KeyboardInterrupt:
        print("\n[停止] 用户手动终止程序。")
    finally:
        # 优雅退出底层进程
        process.terminate()
        process.wait()

if __name__ == "__main__":
    run_wakeword_engine()