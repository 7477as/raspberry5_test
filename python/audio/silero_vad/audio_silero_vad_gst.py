import os
import sys
import subprocess
import numpy as np
import torch

# --- 配置参数 ---
RATE = 16000
CHUNK = 512
THRESHOLD = 0.5

def read_exact(pipe, size):
    """确保从管道中精准读取指定字节数的数据"""
    data = b''
    while len(data) < size:
        packet = pipe.read(size - len(data))
        if not packet:
            break
        data += packet
    return data

def main():
    print("⬇️ 正在从本地缓存加载 PyTorch 原生 Silero VAD 模型...")
    cache_dir = os.path.expanduser("~/.cache/torch/hub/snakers4_silero-vad_master")
    
    try:
        model, utils = torch.hub.load(repo_or_dir=cache_dir,
                                      model='silero_vad',
                                      source='local',
                                      force_reload=False,
                                      trust_repo=True)
    except Exception as e:
        print(f"❌ 加载失败: {e}\n请确保之前已成功运行过离线测试脚本。")
        return

    # 构造 GStreamer 管道 (完美适配 WM8960 的单声道提取)
    gst_cmd = [
        'gst-launch-1.0', '-q',
        'alsasrc', 'device=hw:2,0',
        '!', 'deinterleave', 'name=d',  
        'd.src_0', '!', 'queue',        
        '!', 'audioconvert',
        '!', 'audioresample',
        '!', f'audio/x-raw,format=S16LE,channels=1,rate={RATE}',
        '!', 'fdsink', 'fd=1', 'sync=false'
    ]

    print("🚀 启动底层 GStreamer 硬件麦克风...")
    process = subprocess.Popen(gst_cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)

    print("⏳ 等待硬件电流稳定 (约 0.5 秒)...")

    # 🌟 必须保留的魔法：用于消除麦克风硬件直流偏移的变量
    dc_offset = 0.0

    try:
        bytes_per_read = CHUNK * 2 
        chunk_count = 0
        warmup_chunks = 15  # 丢弃前 0.5 秒的硬件爆音
        
        # 禁用梯度计算，释放 CPU 性能
        with torch.no_grad():
            while True:
                audio_bytes = read_exact(process.stdout, bytes_per_read)
                if not audio_bytes or len(audio_bytes) < bytes_per_read:
                    print("\n⚠️ 音频流中断。")
                    break
                
                chunk_count += 1
                
                # --- 预热期处理 ---
                if chunk_count < warmup_chunks:
                    continue
                elif chunk_count == warmup_chunks:
                    model.reset_states() # 清空初始杂音可能导致的“中毒记忆”
                    print("🟢 麦克风已就绪，开始实时监测！(按 Ctrl+C 停止)\n")
                    continue

                # --- 正常音频解析 ---
                audio_int16 = np.frombuffer(audio_bytes, dtype='<i2')
                audio_float32 = audio_int16.astype(np.float32) / 32768.0
                
                # ==========================================
                # 🌟 EMA 动态消除直流偏移 (DC Offset)
                # ==========================================
                chunk_mean = np.mean(audio_float32)
                dc_offset = 0.1 * chunk_mean + 0.9 * dc_offset 
                audio_float32 = audio_float32 - dc_offset
                
                # 轻微数字增益放大声音
                audio_float32 = np.clip(audio_float32 * 2.0, -1.0, 1.0)
                # ==========================================
                
                max_amp = np.max(np.abs(audio_float32))
                
                # 转换为 PyTorch Tensor 并增加 Batch 维度: (1, 512)
                tensor_chunk = torch.from_numpy(audio_float32).unsqueeze(0)

                # --- 核心推理 ---
                prob = model(tensor_chunk, RATE).item()

                if prob > THRESHOLD:
                    status = "🟢 [有人说话] 🗣️ "
                else:
                    status = "🔴 [ 环境静音 ] 🤫 "
                
                sys.stdout.write(f"\r{status}  |  置信度: {prob:.4f}  |  去偏后音量: {max_amp:.4f} \033[K")
                sys.stdout.flush()

    except KeyboardInterrupt:
        print("\n\n🛑 监测已手动停止。")
    finally:
        process.terminate()
        process.wait()

if __name__ == "__main__":
    main()