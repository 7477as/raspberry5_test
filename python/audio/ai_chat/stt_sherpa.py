# stt_sherpa.py
import sherpa_ncnn
import config
import sys

class SherpaASR:
    def __init__(self):
        print("⏳ 正在加载 Sherpa-ncnn 模型...")
        try:
            self.recognizer = sherpa_ncnn.Recognizer(
                tokens=config.TOKENS,
                encoder_param=config.ENCODER_PARAM,
                encoder_bin=config.ENCODER_BIN,
                decoder_param=config.DECODER_PARAM,
                decoder_bin=config.DECODER_BIN,
                joiner_param=config.JOINER_PARAM,
                joiner_bin=config.JOINER_BIN,
                num_threads=4,
                enable_endpoint_detection=True,
                rule1_min_trailing_silence=2.4,
                rule2_min_trailing_silence=1.2,
                rule3_min_utterance_length=20.0,
            )
            print("✅ Sherpa 模型加载完成")
        except Exception as e:
            print(f"❌ 模型加载失败: {e}")
            sys.exit(1)

    def process(self, audio_samples):
        if len(audio_samples) > 0:
            self.recognizer.accept_waveform(config.SAMPLE_RATE, audio_samples)

    def get_text(self):
        return self.recognizer.text if self.recognizer.text else ""

    def is_endpoint(self):
        return self.recognizer.is_endpoint

    def reset(self):
        """【关键修复】：强行销毁旧流，创建新流，防止 Sherpa 内部状态卡死"""
        try:
            # self.recognizer 是 Python 包装器
            # self.recognizer.recognizer 是底层的 C++ 对象
            self.recognizer.stream = self.recognizer.recognizer.create_stream()
        except Exception as e:
            # 如果你的 sherpa 版本特殊不支持，则回退普通 reset
            print(f"⚠️ 流重建失败，回退普通 reset: {e}")
            self.recognizer.reset()