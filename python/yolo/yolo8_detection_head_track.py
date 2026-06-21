import cv2
import time
import numpy as np
import subprocess
import threading
from ultralytics import YOLO

class RpiCamStream:
    """
    自定义的摄像头读取类，用来替代 cv2.VideoCapture。
    底层通过 subprocess 调用 rpicam-vid 实现完美的自动对焦，并提取裸流。
    """
    def __init__(self, width=1280, height=720, framerate=30):
        self.width = width
        self.height = height
        # YUV420 格式一帧的字节数计算
        self.frame_size = int(width * height * 1.5)
        self.latest_frame = None
        self.is_running = True
        self.lock = threading.Lock()

        # rpicam-vid 命令配置
        cmd = [
            "rpicam-vid",
            "--nopreview",
            "-t", "0",
            "--autofocus-mode", "continuous",
            "--width", str(width),
            "--height", str(height),
            "--framerate", str(framerate),
            "--codec", "yuv420",
            "-o", "-"  # 输出到标准输出
        ]

        print("正在启动 rpicam-vid 进程并开启连续对焦...")
        # 启动子进程，将 stderr 重定向至 DEVNULL 以屏蔽底层多余的打印信息
        self.process = subprocess.Popen(
            cmd, 
            stdout=subprocess.PIPE, 
            stderr=subprocess.DEVNULL, 
            bufsize=self.frame_size * 2
        )

        # 启动后台守护线程不断读取画面 (模拟 GStreamer 的 drop=true)
        # 这样能保证 YOLO 每次拿到的都是最新的画面，不会出现延迟累积
        self.thread = threading.Thread(target=self._update, daemon=True)
        self.thread.start()

        # 等待摄像头预热并获取第一帧
        print("等待摄像头预热与对焦...")
        time.sleep(2)

    def _update(self):
        """后台持续读取视频帧的线程"""
        while self.is_running:
            try:
                # 每次准确读取一帧的数据量
                raw_data = self.process.stdout.read(self.frame_size)
                if len(raw_data) != self.frame_size:
                    print("\n[警告] 无法读取完整的视频帧，视频流可能已结束。")
                    self.is_running = False
                    break

                # 转换为 numpy 数组并转码为 BGR 给 OpenCV 使用
                yuv_data = np.frombuffer(raw_data, dtype=np.uint8)
                yuv_image = yuv_data.reshape((int(self.height * 1.5), self.width))
                bgr_image = cv2.cvtColor(yuv_image, cv2.COLOR_YUV2BGR_I420)

                # 加锁更新最新帧
                with self.lock:
                    self.latest_frame = bgr_image
            except Exception as e:
                print(f"读取视频流出错: {e}")
                self.is_running = False
                break

    def read(self):
        """返回格式与 cv2.VideoCapture 的 read() 保持一致"""
        with self.lock:
            if self.latest_frame is not None:
                return True, self.latest_frame.copy()
            else:
                return False, None

    def isOpened(self):
        return self.is_running

    def release(self):
        """释放资源并关闭子进程"""
        self.is_running = False
        self.process.terminate()
        self.process.wait()


def get_camera():
    """
    初始化摄像头：实例化我们自定义的 RpiCamStream
    """
    cap = RpiCamStream(width=1280, height=720, framerate=30)
    
    if not cap.isOpened():
        print("错误: 无法打开摄像头。")
        exit()
        
    return cap

def draw_detections_with_tracking(frame, result, show_confidence=True):
    """
    绘制带跟踪ID的检测结果
    """
    if result.boxes is None or len(result.boxes) == 0:
        return frame
    
    boxes = result.boxes
    annotated_frame = frame.copy()
    
    # 定义颜色（BGR格式）
    colors = [
        (255, 0, 0),    # 蓝色
        (0, 255, 0),    # 绿色
        (0, 0, 255),    # 红色
        (255, 255, 0),  # 青色
        (255, 0, 255),  # 品红
        (0, 255, 255),  # 黄色
        (128, 0, 128),  # 紫色
        (255, 165, 0),  # 橙色
    ]
    
    # 获取检测数据
    confs = boxes.conf.cpu().numpy()
    classes = boxes.cls.cpu().numpy().astype(int)
    xyxy = boxes.xyxy.cpu().numpy().astype(int)
    
    # 获取跟踪ID（如果存在）
    has_tracking = boxes.id is not None
    if has_tracking:
        track_ids = boxes.id.cpu().numpy().astype(int)
    
    # 绘制每个检测框
    for i, (conf, cls, box) in enumerate(zip(confs, classes, xyxy)):
        x1, y1, x2, y2 = box
        
        # 为每个类别分配颜色
        color = colors[cls % len(colors)]
        
        # 绘制边界框
        cv2.rectangle(annotated_frame, (x1, y1), (x2, y2), color, 2)
        
        # 准备标签文本
        class_name = result.names[cls]
        if has_tracking:
            track_id = track_ids[i]
            if show_confidence:
                label = f"ID:{track_id} {class_name} {conf:.2f}"
            else:
                label = f"ID:{track_id} {class_name}"
        else:
            if show_confidence:
                label = f"{class_name} {conf:.2f}"
            else:
                label = class_name
        
        # 计算文本大小
        (label_width, label_height), baseline = cv2.getTextSize(
            label, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 2
        )
        
        # 绘制标签背景
        cv2.rectangle(
            annotated_frame,
            (x1, y1 - label_height - 5),
            (x1 + label_width, y1),
            color,
            -1
        )
        
        # 绘制标签文字
        cv2.putText(
            annotated_frame, label, (x1, y1 - 5),
            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 2
        )
    
    return annotated_frame

def mode_photo(model, cap):
    """
    模式 1: 拍照并检测物体（不使用跟踪）
    """
    print("\n准备拍照，请将摄像头对准物体...")
    
    # 给摄像头 1.5 秒时间去完成自动对焦和曝光调整
    time.sleep(1.5)
        
    # 读取最终帧
    ret, frame = cap.read()
    if not ret:
        print("无法获取画面帧")
        return

    print("拍照成功！正在进行检测...")
    
    # 进行推理（不使用跟踪）
    results = model(frame)
    result = results[0]
    
    # 获取检测结果
    boxes = result.boxes
    
    print("\n" + "="*50)
    print("      📸 检测结果")
    print("="*50)
    
    if boxes is not None and len(boxes) > 0:
        print(f"检测到 {len(boxes)} 个目标:\n")
        for i, box in enumerate(boxes, 1):
            class_id = int(box.cls[0].item())
            confidence = box.conf[0].item()
            class_name = result.names[class_id]
            x1, y1, x2, y2 = box.xyxy[0].cpu().numpy().astype(int)
            
            print(f"[{i}] 类别ID: {class_id} | 名称: {class_name:15s} | "
                  f"置信度: {confidence*100:.2f}% | "
                  f"位置: ({x1}, {y1}) -> ({x2}, {y2})")
    else:
        print("未检测到任何目标")
    
    print("="*50)
    
    # 绘制检测结果
    annotated_frame = draw_detections_with_tracking(frame.copy(), result, show_confidence=True)
    
    print("\n按任意键关闭图片窗口并退出...")
    cv2.imshow("Detection - Photo Mode", annotated_frame)
    cv2.waitKey(0)
    cv2.destroyAllWindows()

def mode_video_without_tracking(model, cap):
    """
    模式 2a: 实时视频流检测（不使用跟踪）
    """
    print("\n启动实时视频流检测（无跟踪）...")
    print("提示: 在视频窗口处于焦点时，按下 'q' 键退出。")
    
    # FPS 计算相关变量
    fps_counter = 0
    fps_start_time = time.time()
    current_fps = 0

    while True:
        ret, frame = cap.read()
        if not ret:
            print("无法获取画面帧，退出流...")
            break

        # 进行实时推理（不使用跟踪）
        results = model(frame, verbose=False)
        result = results[0]
        
        # 获取检测结果
        boxes = result.boxes
        
        # 统计检测到的数量
        obj_count = len(boxes) if boxes is not None else 0
        
        # 在控制台单行刷新显示结果
        if obj_count > 0:
            # 获取第一个检测到的信息
            first_box = boxes[0]
            class_id = int(first_box.cls[0].item())
            confidence = first_box.conf[0].item()
            class_name = result.names[class_id]
            print(f"\r检测到 {obj_count} 个目标 | 主要目标: {class_name} (ID:{class_id}) | 置信度: {confidence*100:.2f}% | FPS: {current_fps:.1f}", end="")
        else:
            print(f"\r未检测到目标 | FPS: {current_fps:.1f}", end="")
        
        # 绘制检测结果
        annotated_frame = draw_detections_with_tracking(frame.copy(), result, show_confidence=True)
        
        # 在画面左上角显示统计信息
        info_text = f"Objects: {obj_count} | FPS: {current_fps:.1f} | Mode: Detection"
        cv2.putText(annotated_frame, info_text, (10, 30), 
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        
        # 显示画面
        cv2.imshow("Detection - Real-time (No Tracking)", annotated_frame)
        
        # 计算FPS
        fps_counter += 1
        if time.time() - fps_start_time >= 1.0:
            current_fps = fps_counter / (time.time() - fps_start_time)
            fps_counter = 0
            fps_start_time = time.time()

        # 检测按键，按 'q' 退出
        if cv2.waitKey(1) & 0xFF == ord('q'):
            print("\n\n用户手动停止了视频流。")
            break

    cv2.destroyAllWindows()

def mode_video_with_tracking(model, cap, tracker_type='bytetrack'):
    """
    模式 2b/2c: 实时视频流检测（使用指定的跟踪器）
    
    Args:
        model: YOLO模型
        cap: 摄像头对象
        tracker_type: 跟踪器类型，'bytetrack' 或 'botsort'
    """
    # 跟踪器配置
    if tracker_type == 'bytetrack':
        tracker_config = 'bytetrack.yaml'
        tracker_name = "ByteTrack"
        tracker_features = [
            "✓ 速度快，适合实时应用",
            "✓ 计算开销小，适合嵌入式设备",
            "✓ 对低置信度检测有二次匹配",
            "✓ 内存占用低"
        ]
    else:  # botsort
        tracker_config = 'botsort.yaml'
        tracker_name = "BoT-SORT"
        tracker_features = [
            "✓ 精度更高，适合复杂场景",
            "✓ 处理遮挡能力强",
            "✓ 使用卡尔曼滤波和ReID特征",
            "✓ 跟踪更稳定"
        ]
    
    print(f"\n启动实时视频流检测（{tracker_name} 跟踪）...")
    print(f"跟踪器特点:")
    for feature in tracker_features:
        print(f"  {feature}")
    print("\n提示: 在视频窗口处于焦点时，按下 'q' 键退出。")
    print("提示: 每个目标会被分配唯一ID，并持续跟踪")
    
    # 置信度显示阈值（只显示置信度高于此值的检测框）
    CONFIDENCE_THRESHOLD = 0.5
    
    # FPS 计算相关变量
    fps_counter = 0
    fps_start_time = time.time()
    current_fps = 0
    
    # 跟踪统计
    total_tracks = 0
    active_tracks = 0
    
    # 性能统计
    inference_times = []
    
    # 为不同跟踪器设置不同颜色主题
    if tracker_type == 'bytetrack':
        info_color = (0, 255, 0)  # 绿色
        window_title = f"Tracking with {tracker_name}"
    else:
        info_color = (255, 165, 0)  # 橙色
        window_title = f"Tracking with {tracker_name} (Higher Accuracy)"

    while True:
        ret, frame = cap.read()
        if not ret:
            print("无法获取画面帧，退出流...")
            break

        # 记录推理开始时间
        inference_start = time.time()
        
        # 使用指定跟踪器进行跟踪推理
        # persist=True 确保跟踪ID在帧间保持连续
        results = model.track(frame, persist=True, tracker=tracker_config, verbose=False)
        result = results[0]
        
        # 计算推理时间
        inference_time = time.time() - inference_start
        inference_times.append(inference_time)
        if len(inference_times) > 30:  # 只保留最近30帧的记录
            inference_times.pop(0)
        avg_inference_time = np.mean(inference_times) if inference_times else 0
        
        # 获取检测结果
        boxes = result.boxes
        
        # 统计检测到的数量
        obj_count = len(boxes) if boxes is not None else 0
        
        # 获取跟踪信息
        has_tracking = boxes is not None and boxes.id is not None
        if has_tracking:
            track_ids = boxes.id.cpu().numpy().astype(int)
            active_tracks = len(np.unique(track_ids))
            total_tracks = max(total_tracks, active_tracks)
        
        # 在控制台单行刷新显示结果
        if has_tracking and obj_count > 0:
            # 获取第一个跟踪目标的信息
            first_box = boxes[0]
            track_id = int(first_box.id[0].item())
            class_id = int(first_box.cls[0].item())
            confidence = first_box.conf[0].item()
            class_name = result.names[class_id]
            print(f"\r[{tracker_name}] 跟踪中: {active_tracks} 个目标 | 检测: {obj_count} 个目标 | 主要: ID:{track_id} {class_name} | 置信度: {confidence*100:.2f}% | FPS: {current_fps:.1f} | 推理: {avg_inference_time*1000:.1f}ms", end="")
        elif obj_count > 0:
            print(f"\r[{tracker_name}] 检测到 {obj_count} 个目标（跟踪初始化中）| FPS: {current_fps:.1f}", end="")
        else:
            print(f"\r[{tracker_name}] 未检测到目标 | FPS: {current_fps:.1f}", end="")
        
        # 绘制检测结果（带跟踪ID）
        annotated_frame = draw_detections_with_tracking(frame.copy(), result, show_confidence=True)
        
        # 在画面左上角显示统计信息
        info_lines = [
            f"Tracker: {tracker_name}",
            f"FPS: {current_fps:.1f}",
            f"Inference: {avg_inference_time*1000:.1f}ms",
            f"Active Tracks: {active_tracks}",
            f"Detections: {obj_count}",
            f"Total IDs: {total_tracks}",
            f"Conf Thresh: {CONFIDENCE_THRESHOLD}",
            "",
            "Controls:",
            "Q - Quit",
            "I - Show Info",
            "R - Reset Stats",
            "+/- - Adjust Threshold"
        ]
        
        for i, line in enumerate(info_lines):
            y_pos = 30 + i * 25
            cv2.putText(annotated_frame, line, (10, y_pos), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, info_color, 2)
        
        # 显示画面
        cv2.imshow(window_title, annotated_frame)
        
        # 计算FPS
        fps_counter += 1
        if time.time() - fps_start_time >= 1.0:
            current_fps = fps_counter / (time.time() - fps_start_time)
            fps_counter = 0
            fps_start_time = time.time()

        # 检测按键
        key = cv2.waitKey(1) & 0xFF
        if key == ord('q'):
            print(f"\n\n用户手动停止了{tracker_name}视频流。")
            break
        elif key == ord('i'):
            # 按 'i' 键显示跟踪统计信息
            print(f"\n[{tracker_name}统计]")
            print(f"  历史最大同时跟踪数: {total_tracks}")
            print(f"  当前活跃跟踪: {active_tracks}")
            print(f"  平均推理时间: {avg_inference_time*1000:.1f}ms")
            print(f"  当前FPS: {current_fps:.1f}")
        elif key == ord('r'):
            # 按 'r' 键重置跟踪统计
            total_tracks = 0
            print(f"\n[{tracker_name}] 跟踪统计已重置")
        elif key == ord('+') or key == ord('='):
            # 按 '+' 键提高置信度阈值
            CONFIDENCE_THRESHOLD = min(1.0, CONFIDENCE_THRESHOLD + 0.05)
            print(f"\n[{tracker_name}] 置信度阈值提高到: {CONFIDENCE_THRESHOLD:.2f}")
            # 注意：这只是显示阈值，实际跟踪使用的是跟踪器内部的阈值
        elif key == ord('-') or key == ord('_'):
            # 按 '-' 键降低置信度阈值
            CONFIDENCE_THRESHOLD = max(0.0, CONFIDENCE_THRESHOLD - 0.05)
            print(f"\n[{tracker_name}] 置信度阈值降低到: {CONFIDENCE_THRESHOLD:.2f}")

    cv2.destroyAllWindows()

def compare_trackers(model, cap):
    """
    对比 ByteTrack 和 BoT-SORT 的性能
    """
    print("\n" + "="*60)
    print("        跟踪器性能对比测试")
    print("="*60)
    
    trackers = [
        ('bytetrack', 'ByteTrack', (0, 255, 0)),
        ('botsort', 'BoT-SORT', (255, 165, 0))
    ]
    
    results = {}
    
    for tracker_type, tracker_name, color in trackers:
        print(f"\n测试 {tracker_name}...")
        
        # 重置摄像头（确保每个测试从相同状态开始）
        cap.release()
        cap = get_camera()
        
        # 等待稳定
        time.sleep(1)
        
        # 性能统计
        fps_values = []
        inference_times = []
        tracking_counts = []
        
        tracker_config = f'{tracker_type}.yaml'
        test_frames = 100  # 测试100帧
        frame_count = 0
        
        start_time = time.time()
        
        while frame_count < test_frames:
            ret, frame = cap.read()
            if not ret:
                break
            
            # 推理
            inference_start = time.time()
            results_obj = model.track(frame, persist=True, tracker=tracker_config, verbose=False)
            inference_time = time.time() - inference_start
            inference_times.append(inference_time)
            
            # 获取跟踪数量
            if results_obj[0].boxes is not None and results_obj[0].boxes.id is not None:
                tracking_count = len(np.unique(results_obj[0].boxes.id.cpu().numpy().astype(int)))
            else:
                tracking_count = 0
            tracking_counts.append(tracking_count)
            
            frame_count += 1
            
            # 计算FPS
            if frame_count % 10 == 0:
                elapsed = time.time() - start_time
                current_fps = frame_count / elapsed
                fps_values.append(current_fps)
                print(f"  进度: {frame_count}/{test_frames} 帧, FPS: {current_fps:.1f}", end='\r')
        
        # 计算统计数据
        avg_fps = np.mean(fps_values) if fps_values else 0
        avg_inference = np.mean(inference_times) if inference_times else 0
        avg_tracking = np.mean(tracking_counts) if tracking_counts else 0
        
        results[tracker_name] = {
            'avg_fps': avg_fps,
            'avg_inference_ms': avg_inference * 1000,
            'avg_tracking_count': avg_tracking
        }
        
        print(f"\n  {tracker_name} 测试完成 - 平均FPS: {avg_fps:.1f}")
    
    # 显示对比结果
    print("\n" + "="*60)
    print("        对比结果汇总")
    print("="*60)
    print(f"{'跟踪器':<15} {'平均FPS':<12} {'推理时间(ms)':<15} {'平均跟踪数':<12}")
    print("-"*60)
    for tracker_name, stats in results.items():
        print(f"{tracker_name:<15} {stats['avg_fps']:<12.1f} {stats['avg_inference_ms']:<15.1f} {stats['avg_tracking_count']:<12.1f}")
    
    print("\n建议:")
    if results['ByteTrack']['avg_fps'] > results['BoT-SORT']['avg_fps']:
        print("  • ByteTrack 速度更快，适合实时性要求高的场景")
    else:
        print("  • BoT-SORT 速度可能较慢，但精度更高")
    
    if results['BoT-SORT']['avg_tracking_count'] > results['ByteTrack']['avg_tracking_count']:
        print("  • BoT-SORT 能跟踪到更多目标，适合复杂场景")
    else:
        print("  • ByteTrack 在简单场景下表现良好")
    
    print("="*60)
    
    return results

def main():
    print("="*60)
    print("   YOLOv8 + RpiCam + 多跟踪器头部检测系统")
    print("="*60)
    
    # 1. 加载检测模型
    try:
        # 使用头部检测模型 (合并了之前分离的头部模型需求)
        model_path = 'models/yolov8n-head.pt'  
        print(f"正在加载头部检测模型: {model_path}")
        model = YOLO(model_path)
        
        # 测试模型加载
        test_img = np.zeros((640, 640, 3), dtype=np.uint8)
        test_result = model(test_img, verbose=False)[0]
        print("模型加载成功！")
    except Exception as e:
        print(f"加载模型失败: {e}")
        print("请确保模型文件 'models/yolov8n-head.pt' 存在")
        return

    # 2. 用户选择模式 (保留了原本强大的 5 种模式菜单)
    print("\n请选择运行模式：")
    print("1: 拍照检测 (拍一张照片并显示详细的检测结果)")
    print("2: 实时视频检测 (显示实时摄像头画面，无跟踪)")
    print("3: ByteTrack 视频跟踪 (速度快，适合实时应用)")
    print("4: BoT-SORT 视频跟踪 (精度高，适合复杂场景)")
    print("5: 对比测试 (自动测试并对比两种跟踪器性能)")
    
    choice = input("请输入 1-5 并回车: ").strip()

    if choice not in ['1', '2', '3', '4', '5']:
        print("无效的输入，程序退出。")
        return

    # 3. 初始化摄像头
    cap = get_camera()

    # 4. 执行对应模式
    try:
        if choice == '1':
            mode_photo(model, cap)
        elif choice == '2':
            mode_video_without_tracking(model, cap)
        elif choice == '3':
            mode_video_with_tracking(model, cap, tracker_type='bytetrack')
        elif choice == '4':
            mode_video_with_tracking(model, cap, tracker_type='botsort')
        elif choice == '5':
            compare_trackers(model, cap)
    finally:
        # 清理释放摄像头资源
        cap.release()
        cv2.destroyAllWindows()
        print("摄像头资源已释放。")

if __name__ == '__main__':
    main()