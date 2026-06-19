import torch
import wave
import numpy as np
import os

WAV_FILE = "models/audio_silero_vad_file.wav"

def main():
    print("⬇️ 正在从本地缓存加载 PyTorch 原生 Silero VAD 模型 (完全离线)...")
    
    # 指向你上一次已经成功下载的本地缓存目录
    cache_dir = os.path.expanduser("~/.cache/torch/hub/snakers4_silero-vad_master")
    
    try:
        # 增加 source='local'，彻底斩断对 GitHub 的网络请求
        model, utils = torch.hub.load(repo_or_dir=cache_dir,
                                      model='silero_vad',
                                      source='local',
                                      force_reload=False,
                                      trust_repo=True)
    except Exception as e:
        print(f"❌ 本地加载失败: {e}\n可能是缓存不完整。请重试或检查网络。")
        return
        
    print("✅ 模型加载成功！重置模型内部记忆状态...")
    model.reset_states()

    wf = wave.open(WAV_FILE, "rb")
    
    CHUNK = 512
    frame_count = 0
    trigger_count = 0
    
    print(f"\n🚀 开始测试标准人声音频 {WAV_FILE} ...\n")
    
    # 禁用梯度计算，大幅提升推理速度
    with torch.no_grad():
        while True:
            frames = wf.readframes(CHUNK)
            if len(frames) < CHUNK * 2: 
                break
                
            frame_count += 1
            time_ms = frame_count * 32
            
            # 解析二进制音频数据
            audio_int16 = np.frombuffer(frames, dtype='<i2')
            audio_float32 = audio_int16.astype(np.float32) / 32768.0
            max_amp = np.max(np.abs(audio_float32))
            
            # 转换为 PyTorch Tensor: shape (1, 512)
            tensor_chunk = torch.from_numpy(audio_float32).unsqueeze(0)

            # PyTorch 推理
            prob = model(tensor_chunk, 16000).item()
            
            if prob > 0.5:
                trigger_count += 1
                print(f"🟢 [发现人声!] 时间: {time_ms} ms | 置信度: {prob:.4f} | 音量: {max_amp:.4f}")
            elif frame_count % 15 == 0:
                print(f"⚪ (静音扫描) 时间: {time_ms} ms | 置信度: {prob:.4f} | 音量: {max_amp:.4f}")

    print(f"\n✅ 分析完毕。总计分析 {frame_count} 帧，其中检测到人声 {trigger_count} 帧。")

if __name__ == "__main__":
    main()