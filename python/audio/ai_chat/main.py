# main.py
import time
import sys
import numpy as np  # <-- 确保顶部导入 numpy
from audio_capture import AudioCapture
from stt_sherpa import SherpaASR
from llm_qwen import QwenLLM
from tts_edge import EdgeTTS

def main():
    print("=== 🤖 AI 语音助手启动 (GStreamer + Sherpa + Qwen2 + EdgeTTS) ===")
    
    recorder = AudioCapture()
    recognizer = SherpaASR()
    llm = QwenLLM()
    tts = EdgeTTS()

    recorder.start()
    last_text = ""

    try:
        while True:
            audio_chunk = recorder.get_audio_chunk()
            
            if audio_chunk is not None:
                # 【可选诊断】如果你怀疑有时候麦克风完全挂了，开启这段代码
                # if np.max(np.abs(audio_chunk)) == 0.0:
                #     print("\n⚠️ 警告: 麦克风传入全 0 数据，ALSA 硬件可能已锁死！")

                recognizer.process(audio_chunk)
                
                current_text = recognizer.get_text()
                if current_text != last_text:
                    last_text = current_text
                    sys.stdout.write(f"\r👂 听到: {last_text}")
                    sys.stdout.flush()

                if recognizer.is_endpoint():
                    final_text = recognizer.get_text()
                    
                    if len(final_text) > 0:
                        print(f"\n✅ 句子结束，提交处理...")
                        recorder.stop()
                        
                        reply = llm.chat(final_text)
                        tts.speak(reply)
                        
                        recognizer.reset()
                        
                        # 重置后，把队列里积累的废音清空，再重新开始听
                        while recorder.get_audio_chunk() is not None:
                            pass
                            
                        last_text = ""
                        print("\n🎙️ 准备听下一句...")
                        recorder.start()
                    else:
                        print("\n🎙️ 未识别到文字 (可能只是噪音或静音)...")
                        recognizer.reset()
                        
                        # 【关键修复】清空静音积压！把这几秒钟由于没说话而产生的废音频全丢掉
                        while recorder.get_audio_chunk() is not None:
                            pass
                        
            else:
                time.sleep(0.01)

    except KeyboardInterrupt:
        print("\n👋 程序正在退出...")
        if hasattr(recorder, 'cleanup'):
            recorder.cleanup()
        else:
            recorder.stop()
        sys.exit(0)

if __name__ == "__main__":
    main()