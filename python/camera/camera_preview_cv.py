import cv2
import time
from picamera2 import Picamera2
from libcamera import controls

# 1. 初始化 Picamera2 核心
picam = Picamera2()

# 2. 终极配置：采用你系统天生免疫偏色的 "XRGB8888" 4通道格式
# 享受小端序带来的“免通道转换”高性能红利
PREVIEW_SIZE = (1920, 1080)  # 满血 1080p 高清
camera_config = picam.create_video_configuration(
    main={"size": PREVIEW_SIZE, "format": "XRGB8888"}
)
picam.configure(camera_config)

# 3. 设置全局控制：帧率与连续全自动追焦
picam.set_controls({
    "FrameRate": 30,
    "AfMode": controls.AfModeEnum.Continuous  # 开启连续自动对焦，彻底解决模糊
})

# 4. 激活后台硬件流
picam.start()

# 给自动曝光和白平衡 0.5 秒的初始收敛时间
time.sleep(0.5) 
print("=========================================")
print("  Pi5 Pro 满血版原生高清预览已成功启动！")
print("  当前状态：1080p | 连续追焦 | 硬件级零开销色彩")
print("=========================================")

# 创建符合 Wayland 规范的自适应命名窗口
cv2.namedWindow('Raspberry Pi 5 Premium Stream', cv2.WINDOW_NORMAL)

try:
    last_time = time.time()
    while True:
        # 5. 免拷贝、免计算，直接抓取底层最纯正的 BGRX 矩阵视图
        frame = picam.capture_array()
        
        # 💡 此处的 frame 已经色彩完美，你可以直接塞给你的 AI 模型（如 Hailo-8L）
        # target_inference(frame)
        
        # 6. 计算实际帧率并动态更新到窗口标题栏（避免在图像上写字损耗算力）
        current_time = time.time()
        fps = 1 / (current_time - last_time)
        last_time = current_time
        cv2.setWindowTitle(
            'Raspberry Pi 5 Premium Stream', 
            f"FPS: {fps:.1f} | 1080p | Hardware Zero-Copy Color"
        )

        # 7. 直接交付 OpenCV 渲染，尽享丝滑
        cv2.imshow('Raspberry Pi 5 Premium Stream', frame)
        
        # 检测键盘，按 'q' 键优雅退出
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
finally:
    # 8. 稳健地释放硬件中断和内存锁
    picam.stop()
    cv2.destroyAllWindows()
    print("相机流已安全关闭，资源已释放。")