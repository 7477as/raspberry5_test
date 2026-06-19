# tts_edge.py
import asyncio
import edge_tts
import subprocess
import config
import shutil

class EdgeTTS:
    def __init__(self):
        self.voice = config.TTS_VOICE
        self.rate = config.TTS_Rate
        self.volume = config.TTS_Volume
        # 检查是否安装了 mpv
        if not shutil.which("mpv"):
            raise RuntimeError("错误: 未找到 mpv 播放器。请先运行 'sudo apt install mpv' 安装。")

    async def _stream_process(self, text):
        """
        内部异步方法：处理流式生成和管道传输
        """
        communicate = edge_tts.Communicate(text, self.voice, rate=self.rate, volume=self.volume)
        
        # 启动 mpv 进程，设置为从 stdin (标准输入) 读取数据
        # "-" 表示读取管道流
        # --no-cache --profile=low-latency: 最小化延迟
        cmd = ["mpv", "--no-cache", "--no-terminal", "--profile=low-latency", "-"]
        
        process = subprocess.Popen(
            cmd,
            stdin=subprocess.PIPE,     # 允许向 mpv 写入数据
            stdout=subprocess.DEVNULL, # 隐藏 mpv 的输出
            stderr=subprocess.DEVNULL
        )

        try:
            # 循环获取 TTS 数据块
            async for chunk in communicate.stream():
                if chunk["type"] == "audio":
                    if process.stdin:
                        process.stdin.write(chunk["data"])
                        process.stdin.flush() # 立即推送到播放器
        except BrokenPipeError:
            # 播放器可能提前被关闭，忽略此错误
            pass
        except Exception as e:
            print(f"流式播放出错: {e}")
        finally:
            # 数据发送完毕，关闭管道并等待播放结束
            if process.stdin:
                process.stdin.close()
            process.wait()

    def speak(self, text):
        """公开方法：生成语音并播放 (保持方法名不变)"""
        if not text:
            return

        print(f"🔊 正在流式朗读: {text[:15]}...")
        try:
            # 使用 asyncio.run 调用内部的异步流式处理
            asyncio.run(self._stream_process(text))
            
        except Exception as e:
            print(f"❌ TTS 播放失败: {e}")
