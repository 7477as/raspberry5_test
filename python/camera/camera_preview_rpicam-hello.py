import subprocess

# t 0 表示无限期预览，直到手动关闭
try:
    print("启动摄像头预览，按 Ctrl+C 退出...")
    subprocess.run(["rpicam-hello", "-t", "0"])
except KeyboardInterrupt:
    print("预览已结束")