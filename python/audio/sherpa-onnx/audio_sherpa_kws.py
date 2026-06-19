import sherpa_onnx
import os
import sys
import subprocess
import numpy as np

# 屏蔽警告
os.environ['ORT_LOGGING_LEVEL'] = '3'

# 1. 配置文件路径 (确保与你解压的文件夹名称一致)
MODEL_DIR = "./sherpa-onnx-kws-zipformer-wenetspeech-3.3M-2024-01-01"
KEYWORDS_FILE = "./keywords.txt"

# 2. 初始化 Sherpa-ONNX 唤醒引擎
def init_kws():
    print("[系统初始化] 正在加载轻量级中文唤醒模型...")
    keyword_spotter = sherpa_onnx.KeywordSpotter(
        tokens=f"{MODEL_DIR}/tokens.txt",
        encoder=f"{MODEL_DIR}/encoder-epoch-12-avg-2-chunk-16-left-64.onnx",
        decoder=f"{MODEL_DIR}/decoder-epoch-12-avg-2-chunk-16-left-64.onnx",
        joiner=f"{MODEL_DIR}/joiner-epoch-12-avg-2-chunk-16-left-64.onnx",
        num_threads=1,              # 树莓派上 1 个线程足以跑满实时率
        keywords_file=KEYWORDS_FILE,
        provider="cpu"
    )
    return keyword_spotter

def read_exact(pipe, size):
    """确保从管道中精准读取指定字节数的数据"""
    data = b''
    while len(data) < size:
        packet = pipe.read(size - len(data))
        if not packet:
            break
        data += packet
    return data

# 3. 主循环
def main():
    spotter = init_kws()
    
    # === 参数配置 ===
    RATE = 16000                    # Sherpa 模型强制要求 16000Hz
    CHUNK = int(RATE * 0.1)         # 每次读取 0.1 秒的数据 (1600 采样点)
    BYTES_PER_READ = CHUNK * 2      # S16LE 格式每个采样点占 2 字节，每次需读 3200 字节

    # 🌟 GStreamer 魔法管道 🌟
    # 注意：device=hw:2,0 是你之前的 USB 麦克风硬件地址。如果有变，请修改此处。
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
    print("🟢 系统已就绪，麦克风监听中...")
    print("尝试说出你在 keywords.txt 中定义的指令")
    print("="*40 + "\n")

    # 创建一个音频数据流对象，用于连续送入数据
    online_stream = spotter.create_stream()

    try:
        while True:
            # 1. 直接从 GStreamer 底层管道中读取纯净、处理好的 16000Hz 音频字节
            audio_bytes = read_exact(process.stdout, BYTES_PER_READ)
            if not audio_bytes or len(audio_bytes) < BYTES_PER_READ:
                print("\n⚠️ 音频流中断。")
                break

            # 2. 转换为 float32 并归一化到 [-1, 1] 区间
            samples = np.frombuffer(audio_bytes, dtype=np.int16).astype(np.float32) / 32768.0

            # 3. 送入模型特征提取器
            online_stream.accept_waveform(RATE, samples)

            # 4. 执行解码推理
            while spotter.is_ready(online_stream):
                spotter.decode_stream(online_stream)

            # 5. 获取当前帧的识别结果
            result = spotter.get_result(online_stream)
            
            if result:
                print(f"⚡ [触发指令]: {result}")
                
                # --- 在这里可以接入你的结构件控制逻辑 ---
                # 比如：
                # if result == "调整角度":
                #     set_servo_angle(16) 
                
                # 使用 reset_stream 来清空状态，防止同一句话被重复触发
                spotter.reset_stream(online_stream)

    except KeyboardInterrupt:
        print("\n[退出] 引擎已手动关闭。")
    finally:
        # 优雅地杀死底层 C 进程
        process.terminate()
        process.wait()

if __name__ == "__main__":
    main()