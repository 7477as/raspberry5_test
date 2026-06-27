# Raspberry Pi 5 Test Projects (`raspberry5_test`)

本项目是一个针对 **树莓派 5 (Raspberry Pi 5)** 开发的综合性测试与示例代码库。项目涵盖了 C++ 和 Python 两种编程语言，主要用于验证和演示树莓派 5 在机器视觉、音频处理、边缘 AI 计算（如 YOLO 目标检测、大语言模型交互）以及底层系统调度等方面的能力。

---

## 🚀 核心特性

* **边缘 AI 与机器视觉 (YOLO)**: 包含 YOLOv8 的多种追踪脚本（目标检测、人头追踪、行人追踪），并特别支持 **Hailo-8L AI 加速模块** (`yolov8n_h8l.hef`)，实现高帧率推理。
* **智能语音交互 (AI Chat)**: 完整集成了离线语音唤醒 (Open Wake Word / Sherpa-ONNX)、语音活动检测 (Silero VAD)、语音转文本 (Sherpa-NCNN)、本地大语言模型 (Qwen) 以及文本转语音 (Edge TTS) 的端到端对话流。
* **多媒体流处理**: 提供基于 `rpicam-apps`、OpenCV 以及 GStreamer 的摄像头预览和视频编解码 (Encode/Decode) 示例。
* **系统底层测试**: 包含在 Linux 平台上实现高优先级实时线程 (Realtime Thread) 的 C++ 测试代码。

---

## 📁 模块导航

### 🐍 Python 模块 (`/python`)

| 模块大类 | 子模块/脚本 | 功能说明 |
| :--- | :--- | :--- |
| **Audio (音频处理)** | `ai_chat/` | 结合 Qwen LLM、NCNN/ONNX 语音识别和 Edge TTS 的完整 AI 语音助手。 |
| | `open_wake_word/` | 基于 `open_wake_word` 的开源语音唤醒词检测。 |
| | `sherpa-onnx/` | 基于 ONNX 的关键词唤醒/识别 (Keyword Spotting)。 |
| | `silero_vad/` | 语音活动检测 (VAD)，支持处理本地音频文件及 GStreamer 实时流。 |
| **Camera (摄像头)** | `camera_preview_cv.py` | 使用 OpenCV 读取并预览摄像头画面。 |
| | `camera_preview_gst.py` | 使用 GStreamer 管道获取并显示摄像头流。 |
| | `camera_preview_rpicam-hello.py`| 基于树莓派官方 `rpicam-hello` 工具的 Python 调用。 |
| **YOLO (目标检测)** | `yolo8_detection_*.py` | 包含默认检测、人头追踪以及行人追踪脚本。 |
| | `*_hailo.py` | **亮点**：使用 Hailo NPU (`.hef` 模型) 硬件加速 YOLO 目标检测。 |

### ⚙️ C++ 模块 (`/cpp`)

| 模块大类 | 项目/目录 | 功能说明 |
| :--- | :--- | :--- |
| **Camera (摄像头)** | `camera_preview_rpicam-vid` | 使用 C++ 调用 `rpicam-vid` 实现高效的视频流捕获与预览。 |
| **Kernel (内核/系统)** | `realtime_thread` | 演示如何在树莓派上配置并运行具有实时优先级调度的 POSIX 线程。 |
| **Video (视频)** | `video_encode_decode` | 视频硬编解码测试项目，包含完整的 CMake 构建配置。 |

---

## 🛠️ 构建与运行说明

### C++ 项目编译
C++ 目录下的大部分子项目都配备了便捷的 `build_run.sh` 脚本。以摄像头预览为例：
```bash
cd cpp/camera/camera_preview_rpicam-vid
chmod +x build_run.sh
./build_run.sh