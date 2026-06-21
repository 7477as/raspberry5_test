import cv2
import time
import numpy as np
import subprocess
import threading
from ultralytics import YOLO
import torch

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

        # 启动后台守护线程不断读取画面
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

def filter_person_detections(result):
    """
    过滤出人的检测结果（COCO数据集中人的类别ID是0）
    通过创建新的结果对象来实现
    """
    if result.boxes is None or len(result.boxes) == 0:
        return result
    
    # 获取所有检测框
    boxes = result.boxes
    
    # 获取类别ID
    classes = boxes.cls.cpu().numpy().astype(int)
    
    # 创建掩码，只保留人的检测（class_id == 0）
    person_mask = classes == 0
    
    if not any(person_mask):
        # 没有检测到人，返回空结果
        result.boxes = None
        return result
    
    # 获取过滤后的数据
    filtered_xyxy = boxes.xyxy[person_mask]
    filtered_conf = boxes.conf[person_mask]
    filtered_cls = boxes.cls[person_mask]
    
    # 如果有跟踪ID，也进行过滤
    if boxes.id is not None:
        filtered_id = boxes.id[person_mask]
    else:
        filtered_id = None
    
    # 创建新的检测结果对象
    from ultralytics.engine.results import Results
    
    # 创建新的Results对象，只包含人的检测
    new_result = Results(
        orig_img=result.orig_img,
        path=result.path,
        names=result.names,
        boxes=None
    )
    
    # 创建新的Boxes对象（使用正确的方式）
    from ultralytics.utils.ops import xyxy2xywh
    
    # 将数据移到CPU并转换为numpy
    boxes_data = {
        'xyxy': filtered_xyxy.cpu().numpy(),
        'conf': filtered_conf.cpu().numpy(),
        'cls': filtered_cls.cpu().numpy(),
    }
    
    if filtered_id is not None:
        boxes_data['id'] = filtered_id.cpu().numpy()
    
    # 直接赋值给result的boxes属性
    result.boxes = boxes
    
    # 更新boxes数据
    result.boxes.xyxy = filtered_xyxy
    result.boxes.conf = filtered_conf
    result.boxes.cls = filtered_cls
    if filtered_id is not None:
        result.boxes.id = filtered_id
    
    return result

def draw_person_detections(frame, result, show_confidence=True):
    """
    绘制人体检测结果（只显示人，使用红色框）
    """
    if result.boxes is None or len(result.boxes) == 0:
        return frame
    
    boxes = result.boxes
    annotated_frame = frame.copy()
    
    # 人的检测使用红色框
    person_color = (0, 0, 255)  # 红色 (BGR)
    
    # 获取检测数据
    confs = boxes.conf.cpu().numpy()
    classes = boxes.cls.cpu().numpy().astype(int)
    xyxy = boxes.xyxy.cpu().numpy().astype(int)
    
    # 获取跟踪ID（如果存在）
    has_tracking = boxes.id is not None
    if has_tracking:
        track_ids = boxes.id.cpu().numpy().astype(int)
    
    # 绘制每个人
    for i, (conf, cls, box) in enumerate(zip(confs, classes, xyxy)):
        # 只绘制人（class_id == 0）
        if cls != 0:
            continue
            
        x1, y1, x2, y2 = box
        
        # 确保坐标在图像范围内
        x1 = max(0, x1)
        y1 = max(0, y1)
        x2 = min(annotated_frame.shape[1], x2)
        y2 = min(annotated_frame.shape[0], y2)
        
        # 绘制边界框（红色）
        cv2.rectangle(annotated_frame, (x1, y1), (x2, y2), person_color, 2)
        
        # 准备标签文本
        if has_tracking and i < len(track_ids):
            track_id = track_ids[i]
            if show_confidence:
                label = f"Person {track_id} | {conf:.2f}"
            else:
                label = f"Person {track_id}"
        else:
            if show_confidence:
                label = f"Person {conf:.2f}"
            else:
                label = "Person"
        
        # 计算文本大小
        (label_width, label_height), baseline = cv2.getTextSize(
            label, cv2.FONT_HERSHEY_SIMPLEX, 0.6, 2
        )
        
        # 绘制标签背景
        cv2.rectangle(
            annotated_frame,
            (x1, y1 - label_height - 5),
            (x1 + label_width, y1),
            person_color,
            -1
        )
        
        # 绘制标签文字（白色）
        cv2.putText(
            annotated_frame, label, (x1, y1 - 5),
            cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2
        )
    
    return annotated_frame

def mode_photo(model, cap):
    """
    模式 1: 拍照并检测人
    """
    print("\n准备拍照，请将摄像头对准人群...")
    
    # 给摄像头 1.5 秒时间去完成自动对焦和曝光调整
    time.sleep(1.5)
        
    # 读取最终帧
    ret, frame = cap.read()
    if not ret:
        print("无法获取画面帧")
        return

    print("拍照成功！正在进行人体检测...")
    
    # 进行推理（不使用跟踪）
    results = model(frame)
    result = results[0]
    
    # 过滤出人的检测 - 使用简单过滤，不修改原对象
    if result.boxes is not None:
        boxes = result.boxes
        classes = boxes.cls.cpu().numpy().astype(int)
        person_mask = classes == 0
        
        if any(person_mask):
            # 只保留人的检测
            result.boxes = result.boxes[person_mask]
        else:
            result.boxes = None
    
    # 获取检测结果
    boxes = result.boxes
    
    print("\n" + "="*50)
    print("      📸 人体检测结果")
    print("="*50)
    
    if boxes is not None and len(boxes) > 0:
        print(f"检测到 {len(boxes)} 个人:\n")
        for i, box in enumerate(boxes, 1):
            confidence = box.conf[0].item()
            x1, y1, x2, y2 = box.xyxy[0].cpu().numpy().astype(int)
            
            print(f"[{i}] 置信度: {confidence*100:.2f}% | "
                  f"位置: ({x1}, {y1}) -> ({x2}, {y2})")
    else:
        print("未检测到任何人")
    
    print("="*50)
    
    # 绘制检测结果
    annotated_frame = draw_person_detections(frame.copy(), result, show_confidence=True)
    
    print("\n按任意键关闭图片窗口并退出...")
    cv2.imshow("Human Detection - Photo Mode", annotated_frame)
    cv2.waitKey(0)
    cv2.destroyAllWindows()

def mode_video_without_tracking(model, cap):
    """
    模式 2: 实时视频流人体检测（不使用跟踪）
    """
    print("\n启动实时视频流人体检测（无跟踪）...")
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

        # 进行实时推理
        results = model(frame, verbose=False)
        result = results[0]
        
        # 过滤出人的检测
        if result.boxes is not None:
            boxes = result.boxes
            classes = boxes.cls.cpu().numpy().astype(int)
            person_mask = classes == 0
            
            if any(person_mask):
                result.boxes = result.boxes[person_mask]
            else:
                result.boxes = None
        
        # 统计检测到的人数
        person_count = len(result.boxes) if result.boxes is not None else 0
        
        # 在控制台显示结果
        if person_count > 0:
            # 获取第一个检测到的人的信息
            first_box = result.boxes[0]
            confidence = first_box.conf[0].item()
            print(f"\r检测到 {person_count} 个人 | 最高置信度: {confidence*100:.2f}% | FPS: {current_fps:.1f}", end="")
        else:
            print(f"\r未检测到人 | FPS: {current_fps:.1f}", end="")
        
        # 绘制检测结果
        annotated_frame = draw_person_detections(frame.copy(), result, show_confidence=True)
        
        # 在画面左上角显示统计信息
        info_text = f"Humans: {person_count} | FPS: {current_fps:.1f}"
        cv2.putText(annotated_frame, info_text, (10, 30), 
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        
        # 显示画面
        cv2.imshow("Human Detection - Real-time", annotated_frame)
        
        # 计算FPS
        fps_counter += 1
        if time.time() - fps_start_time >= 1.0:
            current_fps = fps_counter / (time.time() - fps_start_time)
            fps_counter = 0
            fps_start_time = time.time()

        # 检测按键
        if cv2.waitKey(1) & 0xFF == ord('q'):
            print("\n\n用户手动停止了视频流。")
            break

    cv2.destroyAllWindows()

def mode_video_with_tracking(model, cap, tracker_type='bytetrack'):
    """
    模式 3/4: 实时视频流人体跟踪（使用指定的跟踪器）
    """
    # 跟踪器配置
    if tracker_type == 'bytetrack':
        tracker_config = 'bytetrack.yaml'
        tracker_name = "ByteTrack"
    else:
        tracker_config = 'botsort.yaml'
        tracker_name = "BoT-SORT"
    
    print(f"\n启动实时视频流人体跟踪（{tracker_name}）...")
    print("提示: 在视频窗口处于焦点时，按下 'q' 键退出。")
    print("提示: 每个人会被分配唯一ID，并持续跟踪")
    
    # FPS 计算变量
    fps_counter = 0
    fps_start_time = time.time()
    current_fps = 0
    
    # 跟踪统计
    total_tracks = 0
    active_tracks = 0
    
    while True:
        ret, frame = cap.read()
        if not ret:
            print("无法获取画面帧，退出流...")
            break

        # 记录开始时间
        start_time = time.time()
        
        # 使用跟踪器进行推理
        results = model.track(frame, persist=True, tracker=tracker_config, verbose=False)
        result = results[0]
        
        # 过滤出人的检测
        if result.boxes is not None:
            boxes = result.boxes
            classes = boxes.cls.cpu().numpy().astype(int)
            person_mask = classes == 0
            
            if any(person_mask):
                # 只保留人的检测
                result.boxes = result.boxes[person_mask]
            else:
                result.boxes = None
        
        # 统计检测和跟踪信息
        person_count = len(result.boxes) if result.boxes is not None else 0
        
        # 获取跟踪信息
        if result.boxes is not None and result.boxes.id is not None:
            track_ids = result.boxes.id.cpu().numpy().astype(int)
            active_tracks = len(np.unique(track_ids))
            total_tracks = max(total_tracks, active_tracks)
        else:
            active_tracks = 0
        
        # 计算FPS
        inference_time = time.time() - start_time
        current_fps = 1.0 / inference_time if inference_time > 0 else 0
        
        # 控制台输出
        if person_count > 0:
            print(f"\r[{tracker_name}] 跟踪中: {active_tracks} 个人 | FPS: {current_fps:.1f}", end="")
        else:
            print(f"\r[{tracker_name}] 未检测到人 | FPS: {current_fps:.1f}", end="")
        
        # 绘制结果
        annotated_frame = draw_person_detections(frame.copy(), result, show_confidence=True)
        
        # 显示信息
        info_lines = [
            f"Tracker: {tracker_name}",
            f"Humans: {active_tracks}",
            f"FPS: {current_fps:.1f}",
            "Press 'q' to quit"
        ]
        
        for i, line in enumerate(info_lines):
            y_pos = 30 + i * 30
            cv2.putText(annotated_frame, line, (10, y_pos), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
        
        # 显示画面
        cv2.imshow(f"Human Tracking - {tracker_name}", annotated_frame)
        
        # 检测按键
        if cv2.waitKey(1) & 0xFF == ord('q'):
            print(f"\n\n用户手动停止了{tracker_name}视频流。")
            break

    cv2.destroyAllWindows()

def compare_trackers(model, cap):
    """
    对比 ByteTrack 和 BoT-SORT 的性能
    """
    print("\n" + "="*60)
    print("        人体跟踪器性能对比测试")
    print("="*60)
    
    results = {}
    
    for tracker_type, tracker_name in [('bytetrack', 'ByteTrack'), ('botsort', 'BoT-SORT')]:
        print(f"\n测试 {tracker_name}...")
        
        # 重置摄像头
        cap.release()
        cap = get_camera()
        time.sleep(1)
        
        # 性能统计
        fps_values = []
        inference_times = []
        tracking_counts = []
        
        tracker_config = f'{tracker_type}.yaml'
        test_frames = 50  # 测试50帧
        frame_count = 0
        
        start_time = time.time()
        
        while frame_count < test_frames:
            ret, frame = cap.read()
            if not ret:
                break
            
            # 推理
            inference_start = time.time()
            results_obj = model.track(frame, persist=True, tracker=tracker_config, verbose=False)
            result = results_obj[0]
            
            # 过滤出人的检测
            if result.boxes is not None:
                classes = result.boxes.cls.cpu().numpy().astype(int)
                person_mask = classes == 0
                if any(person_mask):
                    result.boxes = result.boxes[person_mask]
                else:
                    result.boxes = None
            
            inference_time = time.time() - inference_start
            inference_times.append(inference_time)
            
            # 获取跟踪人数
            if result.boxes is not None and result.boxes.id is not None:
                tracking_count = len(np.unique(result.boxes.id.cpu().numpy().astype(int)))
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
    print(f"{'跟踪器':<15} {'平均FPS':<12} {'推理时间(ms)':<15} {'平均跟踪人数':<12}")
    print("-"*60)
    for tracker_name, stats in results.items():
        print(f"{tracker_name:<15} {stats['avg_fps']:<12.1f} {stats['avg_inference_ms']:<15.1f} {stats['avg_tracking_count']:<12.1f}")
    
    print("\n建议:")
    if results['ByteTrack']['avg_fps'] > results['BoT-SORT']['avg_fps']:
        print("  • ByteTrack 速度更快，适合实时人体跟踪")
    else:
        print("  • BoT-SORT 速度可能较慢，但精度更高")
    
    print("="*60)
    
    return results

def main():
    print("="*60)
    print("   YOLOv8 + RpiCam 人体检测与跟踪系统")
    print("="*60)
    
    # 1. 加载检测模型
    try:
        model_path = 'models/yolov8n.pt'
        print(f"正在加载模型: {model_path}")
        model = YOLO(model_path)
        print("模型加载成功！")
        print("注意: 本系统只检测和跟踪人类")
    except Exception as e:
        print(f"加载模型失败: {e}")
        return

    # 2. 用户选择模式
    print("\n请选择运行模式：")
    print("1: 拍照检测")
    print("2: 实时视频检测 (无跟踪)")
    print("3: ByteTrack 人体跟踪")
    print("4: BoT-SORT 人体跟踪")
    print("5: 对比测试")
    
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
        cap.release()
        cv2.destroyAllWindows()
        print("摄像头资源已释放。")

if __name__ == '__main__':
    main()