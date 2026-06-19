# audio_capture.py (修复版)
import sys
import threading
import queue
import wave
import numpy as np
import gi
import config

gi.require_version('Gst', '1.0')
from gi.repository import Gst, GLib

class AudioCapture:
    def __init__(self):
        Gst.init(None)
        self.audio_queue = queue.Queue()
        self.loop = GLib.MainLoop()
        self.pipeline = None
        self.bus = None
        self.is_running = False
        self.wav_file = None
        self.thread = None  # 记录线程

    def _create_pipeline(self):
        # 修复1: 暂时移除 webrtcdsp 以保证稳定性（单麦克风一般不需要它）
        # 修复2: 增大 queue 和 appsink 的缓冲区，并将 drop 设为 False 防止丢帧
        pipeline_str = (
            f"alsasrc device={config.MIC_DEVICE} ! "
            "audioconvert ! audioresample ! "
            f"audio/x-raw,channels={config.CHANNELS},rate={config.SAMPLE_RATE},format=F32LE ! "
            "queue max-size-time=0 max-size-bytes=0 max-size-buffers=100 ! " 
            "appsink name=sink emit-signals=True max-buffers=100 drop=False sync=False"
        )
        print("管道字符串:", pipeline_str)
        return Gst.parse_launch(pipeline_str)

    def _on_bus_message(self, bus, message):
        t = message.type
        if t == Gst.MessageType.ERROR:
            err, debug = message.parse_error()
            print(f"❌ 错误: {err}, {debug}")
        elif t == Gst.MessageType.WARNING:
            err, debug = message.parse_warning()
            print(f"⚠️ 警告: {err}, {debug}")
        return True

    def _on_new_sample(self, sink, data):
        sample = sink.emit('pull-sample')
        buf = sample.get_buffer()
        result, map_info = buf.map(Gst.MapFlags.READ)
        if result:
            audio_data = np.frombuffer(map_info.data, dtype=np.float32)
            self.audio_queue.put(audio_data)
            
            # 调试录音文件写入
            if self.wav_file:
                audio_int16 = (audio_data * 32767).clip(-32768, 32767).astype(np.int16)
                self.wav_file.writeframes(audio_int16.tobytes())
                
            buf.unmap(map_info)
        return Gst.FlowReturn.OK

    def start(self):
        if self.is_running:
            return
            
        # 修复3: 如果 pipeline 已经存在，说明是“恢复录音”，只需改变状态即可
        if self.pipeline:
            self.pipeline.set_state(Gst.State.PLAYING)
            self.is_running = True
            # 清空之前可能残留的旧队列数据，防止读到历史静音
            while not self.audio_queue.empty():
                self.audio_queue.get_nowait()
            print("▶️ 录音已恢复...")
            return

        print(f"🎤 首次启动录音 (设备: {config.MIC_DEVICE})...")

        if hasattr(config, 'DEBUG_WAV_PATH') and config.DEBUG_WAV_PATH:
            try:
                self.wav_file = wave.open(config.DEBUG_WAV_PATH, 'wb')
                self.wav_file.setnchannels(config.CHANNELS)
                self.wav_file.setsampwidth(2)
                self.wav_file.setframerate(config.SAMPLE_RATE)
            except Exception as e:
                print(f"⚠️ 无法创建 WAV: {e}")

        try:
            self.pipeline = self._create_pipeline()
            sink = self.pipeline.get_by_name('sink')
            sink.connect('new-sample', self._on_new_sample, None)

            self.bus = self.pipeline.get_bus()
            self.bus.add_signal_watch()
            self.bus.connect('message', self._on_bus_message)

            self.pipeline.set_state(Gst.State.PLAYING)
            self.is_running = True
            
            # 只在首次启动时开启主循环线程
            self.thread = threading.Thread(target=self.loop.run)
            self.thread.daemon = True
            self.thread.start()
            print("✅ 录音已开始")

        except Exception as e:
            print(f"❌ 启动失败: {e}")
            sys.exit(1)

    def stop(self):
        """修复4: 暂停录音而不是彻底销毁，解决线程泄漏和麦克风锁死问题"""
        if self.is_running and self.pipeline:
            self.pipeline.set_state(Gst.State.PAUSED)
            self.is_running = False
            print("⏸️ 录音已暂停 (避免录入 TTS 回音)")
            
    def cleanup(self):
        """如果程序退出，才需要真正调用清理"""
        if self.pipeline:
            self.pipeline.set_state(Gst.State.NULL)
            self.loop.quit()
        if self.wav_file:
            self.wav_file.close()

    def get_audio_chunk(self):
        try:
            return self.audio_queue.get(block=False)
        except queue.Empty:
            return None