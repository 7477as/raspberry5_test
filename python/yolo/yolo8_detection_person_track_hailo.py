import os
os.environ["QT_QPA_PLATFORM"] = "xcb" # 强制 OpenCV 使用兼容的显示后端，减少警告
import cv2
import time
import numpy as np
import subprocess
import threading
import supervision as sv
from hailo_platform import (
    HEF, VDevice, HailoStreamInterface, ConfigureParams, 
    InputVStreamParams, OutputVStreamParams, FormatType, InferVStreams
)

class RpiCamStream:
    """
    自定义的摄像头读取类，用来替代 cv2.VideoCapture。
    底层通过 subprocess 调用 rpicam-vid 实现完美的自动对焦，并提取裸流。
    """
    def __init__(self, width=1280, height=720, framerate=30):
        self.width = width
        self.height = height
        self.frame_size = int(width * height * 1.5)
        self.latest_frame = None
        self.is_running = True
        self.lock = threading.Lock()

        cmd = [
            "rpicam-vid", "--nopreview", "-t", "0",
            "--autofocus-mode", "continuous",
            "--width", str(width), "--height", str(height),
            "--framerate", str(framerate), "--codec", "yuv420", "-o", "-"
        ]

        print("正在启动 rpicam-vid 进程并开启连续对焦...")
        self.process = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, bufsize=self.frame_size * 2
        )

        self.thread = threading.Thread(target=self._update, daemon=True)
        self.thread.start()
        print("等待摄像头预热与对焦...")
        time.sleep(2)

    def _update(self):
        while self.is_running:
            try:
                raw_data = self.process.stdout.read(self.frame_size)
                if len(raw_data) != self.frame_size:
                    self.is_running = False
                    break
                yuv_data = np.frombuffer(raw_data, dtype=np.uint8)
                yuv_image = yuv_data.reshape((int(self.height * 1.5), self.width))
                bgr_image = cv2.cvtColor(yuv_image, cv2.COLOR_YUV2BGR_I420)
                with self.lock:
                    self.latest_frame = bgr_image
            except Exception as e:
                self.is_running = False
                break

    def read(self):
        with self.lock:
            if self.latest_frame is not None:
                return True, self.latest_frame.copy()
            else:
                return False, None

    def isOpened(self):
        return self.is_running

    def release(self):
        self.is_running = False
        self.process.terminate()
        self.process.wait()


class HailoYOLOv8:
    """
    封装 HailoRT Python API，实现类似 Ultralytics YOLO 的调用接口 (高性能常驻版)
    """
    def __init__(self, hef_path, input_shape=(640, 640)):
        self.input_shape = input_shape
        self.target = VDevice()
        self.hef = HEF(hef_path)
        
        self.configure_params = ConfigureParams.create_from_hef(self.hef, interface=HailoStreamInterface.PCIe)
        self.network_groups = self.target.configure(self.hef, self.configure_params)
        self.network_group = self.network_groups[0]
        self.network_group_params = self.network_group.create_params()
        
        self.input_vstreams_params = InputVStreamParams.make(self.network_group, format_type=FormatType.UINT8)
        self.output_vstreams_params = OutputVStreamParams.make(self.network_group, format_type=FormatType.FLOAT32)
        
        self.input_vstream_info = self.hef.get_input_vstream_infos()[0]

        # 【核心优化】：在初始化时就建立管道并激活 NPU，保持常驻状态
        self.infer_pipeline = InferVStreams(self.network_group, self.input_vstreams_params, self.output_vstreams_params)
        self.infer_pipeline.__enter__() # 手动进入上下文
        
        self.activation_ctx = self.network_group.activate(self.network_group_params)
        self.activation_ctx.__enter__() # 手动激活模型

    def release(self):
        """安全释放 NPU 硬件资源（熄火）"""
        if hasattr(self, 'activation_ctx'):
            self.activation_ctx.__exit__(None, None, None)
        if hasattr(self, 'infer_pipeline'):
            self.infer_pipeline.__exit__(None, None, None)

    def predict(self, frame):
        """进行推理，并返回 supervision 兼容的 Detections 对象"""
        orig_h, orig_w = frame.shape[:2]
        
        # 前处理：BGR转RGB -> 缩放 -> 增加 Batch 维度
        img = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        img_resized = cv2.resize(img, self.input_shape)
        img_expanded = np.expand_dims(img_resized, axis=0)
        
        # 【核心优化】：直接塞入数据进行推理，省去极其耗时的管道开启和模型激活过程
        input_dict = {self.input_vstream_info.name: img_expanded}
        output_dict = self.infer_pipeline.infer(input_dict)
        
        boxes, confs, class_ids = [], [], []
        parsed = False
        
        # 智能遍历 Hailo 输出层
        for k, v in output_dict.items():
            
            if isinstance(v, list) and len(v) > 0 and isinstance(v[0], list) and len(v[0]) >= 80:
                person_boxes = v[0][0] 
                for box in person_boxes:
                    if len(box) >= 5:
                        ymin, xmin, ymax, xmax, conf = box[:5]
                        if conf > 0.25:
                            boxes.append([int(xmin * orig_w), int(ymin * orig_h), int(xmax * orig_w), int(ymax * orig_h)])
                            confs.append(conf)
                            class_ids.append(0)
                parsed = True
                break 
                
            try:
                v_arr = np.asarray(v, dtype=np.float32)
            except Exception:
                continue

            if v_arr.ndim == 3 and v_arr.shape[-1] == 6:
                for det in v_arr[0]:
                    ymin, xmin, ymax, xmax, conf, cls_id = det
                    if conf > 0.25 and int(cls_id) == 0:
                        boxes.append([int(xmin * orig_w), int(ymin * orig_h), int(xmax * orig_w), int(ymax * orig_h)])
                        confs.append(conf)
                        class_ids.append(int(cls_id))
                parsed = True
                break
                
            elif v_arr.ndim == 4 and v_arr.shape[-1] == 5:
                if v_arr.shape[1] > 0:
                    person_boxes = v_arr[0, 0, :, :] 
                    for box in person_boxes:
                        ymin, xmin, ymax, xmax, conf = box
                        if conf > 0.25:
                            boxes.append([int(xmin * orig_w), int(ymin * orig_h), int(xmax * orig_w), int(ymax * orig_h)])
                            confs.append(conf)
                            class_ids.append(0)
                parsed = True
                break

        if not parsed:
            b_arr, s_arr, c_arr = None, None, None
            for k, v in output_dict.items():
                try:
                    v_arr = np.asarray(v)
                except Exception:
                    continue
                if v_arr.shape[-1] == 4: b_arr = v_arr[0]
                elif v_arr.shape[-1] == 1 and "class" not in k.lower(): s_arr = v_arr[0]
                elif v_arr.shape[-1] == 1 and "class" in k.lower(): c_arr = v_arr[0]
            
            if b_arr is not None and s_arr is not None:
                if c_arr is None: c_arr = np.zeros_like(s_arr)
                for i in range(len(b_arr)):
                    conf = s_arr[i][0]
                    cls_id = int(c_arr[i][0])
                    if conf > 0.25 and cls_id == 0:
                        ymin, xmin, ymax, xmax = b_arr[i]
                        boxes.append([int(xmin * orig_w), int(ymin * orig_h), int(xmax * orig_w), int(ymax * orig_h)])
                        confs.append(conf)
                        class_ids.append(cls_id)

        if len(boxes) == 0:
            return sv.Detections.empty()
            
        return sv.Detections(
            xyxy=np.array(boxes, dtype=np.float32),
            confidence=np.array(confs, dtype=np.float32),
            class_id=np.array(class_ids, dtype=int)
        )


def get_camera():
    cap = RpiCamStream(width=1280, height=720, framerate=30)
    if not cap.isOpened():
        print("错误: 无法打开摄像头。")
        exit()
    return cap


def draw_detections(frame, detections):
    """使用 supervision 绘制边界框和标签"""
    box_annotator = sv.BoxAnnotator(color=sv.Color.RED)
    label_annotator = sv.LabelAnnotator(color=sv.Color.RED, text_color=sv.Color.WHITE)
    
    labels = []
    for i in range(len(detections)):
        if detections.tracker_id is not None:
            labels.append(f"Person {detections.tracker_id[i]} | {detections.confidence[i]:.2f}")
        else:
            labels.append(f"Person {detections.confidence[i]:.2f}")
            
    annotated_frame = box_annotator.annotate(scene=frame.copy(), detections=detections)
    annotated_frame = label_annotator.annotate(scene=annotated_frame, detections=detections, labels=labels)
    return annotated_frame


def mode_photo(model, cap):
    """模式 1: 拍照并检测人"""
    print("\n准备拍照，请将摄像头对准人群...")
    time.sleep(1.5)
    
    ret, frame = cap.read()
    if not ret: return

    print("拍照成功！正在调用 Hailo-8L 进行人体检测...")
    detections = model.predict(frame)
    
    print("\n" + "="*50)
    print("      📸 Hailo 人体检测结果")
    print("="*50)
    
    if len(detections) > 0:
        print(f"检测到 {len(detections)} 个人:\n")
        for i, (xyxy, _, conf, _, _, _) in enumerate(detections, 1):
            print(f"[{i}] 置信度: {conf*100:.2f}% | 位置: ({int(xyxy[0])}, {int(xyxy[1])}) -> ({int(xyxy[2])}, {int(xyxy[3])})")
    else:
        print("未检测到任何人")
    
    annotated_frame = draw_detections(frame, detections)
    cv2.imshow("Hailo Detection - Photo Mode", annotated_frame)
    cv2.waitKey(0)


def mode_video_without_tracking(model, cap):
    """模式 2: 实时视频流人体检测（无 Track）"""
    print("\n启动实时视频流人体检测...")
    fps_start_time = time.time()
    fps_counter = 0
    current_fps = 0

    while True:
        ret, frame = cap.read()
        if not ret: break

        detections = model.predict(frame)
        person_count = len(detections)
        
        annotated_frame = draw_detections(frame, detections)
        
        cv2.putText(annotated_frame, f"Humans: {person_count} | FPS: {current_fps:.1f} (Hailo)", (10, 30), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        cv2.imshow("Hailo Real-time", annotated_frame)
        
        fps_counter += 1
        if time.time() - fps_start_time >= 1.0:
            current_fps = fps_counter / (time.time() - fps_start_time)
            fps_counter = 0
            fps_start_time = time.time()

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break


def mode_video_with_tracking(model, cap):
    """模式 3: 实时视频流人体 Track (通过 supervision 的 ByteTrack 实现)"""
    print("\n启动 Hailo 实时视频流人体 Track (ByteTrack)...")
    
    tracker = sv.ByteTrack()
    
    fps_start_time = time.time()
    fps_counter = 0
    current_fps = 0
    active_tracks = 0

    while True:
        ret, frame = cap.read()
        if not ret: break

        # 1. Hailo 推理获取检测框
        detections = model.predict(frame)
        
        # 2. 将检测结果喂给 Tracker
        tracked_detections = tracker.update_with_detections(detections)
        
        if tracked_detections.tracker_id is not None:
            active_tracks = len(np.unique(tracked_detections.tracker_id))
        else:
            active_tracks = 0
            
        annotated_frame = draw_detections(frame, tracked_detections)
        
        # 绘制统计
        info_lines = [
            "Tracker: ByteTrack",
            f"Humans: {active_tracks}",
            f"FPS: {current_fps:.1f} (Hailo)"
        ]
        for i, line in enumerate(info_lines):
            cv2.putText(annotated_frame, line, (10, 30 + i * 30), 
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
                        
        cv2.imshow("Hailo Tracking - ByteTrack", annotated_frame)
        
        fps_counter += 1
        if time.time() - fps_start_time >= 1.0:
            current_fps = fps_counter / (time.time() - fps_start_time)
            fps_counter = 0
            fps_start_time = time.time()

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break


def main():
    print("="*60)
    print("   YOLOv8 + RpiCam + Hailo-8L 人体检测与 Track 系统")
    print("="*60)
    
    try:
        # 确保这里的路径指向你正确下载的 .hef 模型！
        model_path = 'models/yolov8n_h8l.hef'
        print(f"正在加载 Hailo 模型: {model_path}")
        model = HailoYOLOv8(model_path)
        print("Hailo 模型加载成功！")
    except Exception as e:
        print(f"加载模型失败: {e}\n请确保你已安装 hailo_platform 并且拥有 .hef 文件。")
        return

    print("\n请选择运行模式：")
    print("1: 拍照检测")
    print("2: 实时视频检测 (无 Track)")
    print("3: ByteTrack 人体 Track")
    choice = input("请输入 1-3 并回车: ").strip()

    if choice not in ['1', '2', '3']:
        print("无效输入，退出。")
        return

    cap = get_camera()

    try:
        if choice == '1': mode_photo(model, cap)
        elif choice == '2': mode_video_without_tracking(model, cap)
        elif choice == '3': mode_video_with_tracking(model, cap)
    finally:
        cap.release()
        
        # 【新增】：退出时释放 Hailo 硬件模型资源
        if 'model' in locals():
            model.release() 
            
        cv2.destroyAllWindows()
        print("系统退出，资源释放。")

if __name__ == '__main__':
    main()