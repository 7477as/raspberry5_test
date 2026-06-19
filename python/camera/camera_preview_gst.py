import cv2

# 构建专业的多媒体管道：
# 1. 调用官方硬件底层：libcamerasrc 并开启连续自动对焦
# 2. 限定硬件输出格式：NV12（高效的 YUV 格式）
# 3. 通过 videoconvert 转换为 OpenCV 识别的 BGR 格式
# 4. 送入 appsink，丢弃过期帧以保证实时性
gst_pipeline = (
    "libcamerasrc af-mode=continuous ! "
    "video/x-raw,width=1920,height=1080,frametime=30/1,format=NV12 ! "
    "videoflip method=horizontal-flip ! "
    "videoconvert ! "
    "video/x-raw,format=BGR ! "
    "appsink drop=true max-buffers=1 emit-signals=true"
)

# 使用 GStreamer 后端打开视频流
cap = cv2.VideoCapture(gst_pipeline, cv2.CAP_GSTREAMER)

if not cap.isOpened():
    print("错误：GStreamer 管道无法启动，请检查系统底层依赖。")
    exit()

print("GStreamer 管道启动成功，正在预览... 按 'q' 键退出。")

while True:
    ret, frame = cap.read()
    if not ret:
        print("错误：无法接收画面帧。")
        break
        
    cv2.imshow("GStreamer Zero-Copy Preview", frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()