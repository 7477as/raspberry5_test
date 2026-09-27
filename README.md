# Raspberry Pi 5 Test Projects (`raspberry5_test`)

本项目是一个针对 **树莓派 5 (Raspberry Pi 5)** 开发的综合性测试与示例代码库。项目涵盖了 C、C++ 和 Python 三种编程语言，主要用于验证和演示树莓派 5 在机器视觉、音频处理、边缘 AI 计算（如 YOLO 目标检测、大语言模型交互）、蓝牙 / 网络通信以及底层系统调度等方面的能力。

---

## 🚀 核心特性

* **边缘 AI 与机器视觉 (YOLO)**: 包含 YOLOv8 的多种追踪脚本（默认检测、人头追踪、行人追踪），并特别支持 **Hailo-8L AI 加速模块** (`yolov8n_h8l.hef`)，实现高帧率推理。
* **智能语音交互 (AI Chat)**: 完整集成了离线语音唤醒 (Open Wake Word / Sherpa-ONNX)、语音活动检测 (Silero VAD)、语音转文本 (Sherpa-NCNN)、本地大语言模型 (Qwen) 以及文本转语音 (Edge TTS) 的端到端对话流。
* **多媒体流处理**: 提供基于 `rpicam-apps`、OpenCV、GStreamer 的摄像头预览和视频编解码 (Encode/Decode) 示例，并包含 HTTP / WebSocket 拉流的 client / server 示例。
* **无线与网络通信**: 提供 BLE GATT client / server 示例，以及 WebSocket client / server 和 HTTP server（含文件上传）示例。
* **嵌入式 C 中间件 (stdf)**: `c/middleware/` 下提供两套面向 MCU 风格的 Linux 可运行示例 —— `stdf_os`（OS 适配 + mailbox + 延迟消息 + 自重链定时器）与 `stdf_mvc`（发布订阅中间件，静态内存、零堆分配）。
* **系统底层测试**: 包含在 Linux 平台上实现高优先级实时线程 (Realtime Thread) 的 C++ 测试代码。

---

## 📁 模块导航

### 🐍 Python 模块 (`/python`)

| 模块大类 | 子模块 / 脚本 | 功能说明 |
| :--- | :--- | :--- |
| **Audio (音频处理)** | `audio/ai_chat/` | 结合 Qwen LLM、NCNN/ONNX 语音识别和 Edge TTS 的完整 AI 语音助手。 |
| | `audio/open_wake_word/` | 基于 `open_wake_word` 的开源语音唤醒词检测。 |
| | `audio/sherpa-onnx/` | 基于 ONNX 的关键词唤醒/识别 (Keyword Spotting)。 |
| | `audio/silero_vad/` | 语音活动检测 (VAD)，支持处理本地音频文件及 GStreamer 实时流。 |
| **Bluetooth (蓝牙)** | `bluetooth/ble/gattc/` | BLE GATT client 示例，扫描并连接外设、读写特征值。 |
| | `bluetooth/ble/gatts/` | BLE GATT server 示例（含 `noddy` 演示服务），广播并响应客户端读写。 |
| **Camera (摄像头)** | `camera/camera_preview_cv.py` | 使用 OpenCV 读取并预览摄像头画面。 |
| | `camera/camera_preview_gst.py` | 使用 GStreamer 管道获取并显示摄像头流。 |
| | `camera/camera_preview_rpicam-hello.py` | 基于树莓派官方 `rpicam-hello` 工具的 Python 调用。 |
| **Net (网络)** | `net/http/` | 基于标准库 `http.server` 的 HTTP server，支持文件上传（`uploads/` 目录）。 |
| | `net/websocket/` | `websocket-server` / `websocket-client` 双向消息示例。 |
| **Video (视频)** | `video/` | 视频处理脚本占位（暂为空目录）。 |
| **YOLO (目标检测)** | `yolo/yolo8_detection_default_track.py` | 默认检测 + 通用目标追踪脚本。 |
| | `yolo/yolo8_detection_head_track.py` | 人头追踪脚本。 |
| | `yolo/yolo8_detection_person_track.py` | 行人追踪脚本。 |
| | `yolo/yolo8_detection_person_track_hailo.py` | **亮点**：使用 Hailo NPU (`yolov8n_h8l.hef`) 硬件加速的行人追踪。 |

### ⚙️ C++ 模块 (`/cpp`)

| 模块大类 | 项目 / 目录 | 功能说明 |
| :--- | :--- | :--- |
| **Camera (摄像头)** | `camera/camera_preview_rpicam-vid` | 使用 C++ 调用 `rpicam-vid` 实现高效的视频流捕获与预览。 |
| **Kernel (内核/系统)** | `kernel/realtime_thread` | 演示如何在树莓派上配置并运行具有实时优先级调度的 POSIX 线程。 |
| **Video (视频)** | `video/video_encode_decode` | 视频硬编解码测试项目，包含完整的 CMake 构建配置。 |
| | `video/video_play_gstreamer` | 基于 GStreamer 管道的视频播放示例（带 CMake / `build_run.sh`）。 |
| | `video/video_play_http` | HTTP 视频流 server / client 拉流示例（含 `server/` 与 `client/` 两个子项目）。 |

### 🔧 C 模块 (`/c`)

| 模块大类 | 项目 / 目录 | 功能说明 |
| :--- | :--- | :--- |
| **Middleware (中间件)** | `middleware/stdf_os` | stdf 风格的 OS 适配 demo：mailbox + 延迟消息 + 自重链定时器，按键状态机识别 SINGLE / DOUBLE / TRIPLE / LONG。 |
| | `middleware/stdf_mvc` | 发布订阅中间件，静态内存、零默认堆分配，4 个公开 API + per-subject cache，4 个子系统（dm / net / camera / ui）。 |

---

## 🛠️ 构建与运行说明

### C++ 项目编译

C++ 目录下的大部分子项目都配备了便捷的 `build_run.sh` 脚本。以摄像头预览为例：

```bash
cd cpp/camera/camera_preview_rpicam-vid
chmod +x build_run.sh
./build_run.sh
```

其他 C++ 子项目的构建流程相同，进入对应目录执行 `./build_run.sh` 即可（默认 Release 构建并运行；可通过 `BUILD_TYPE=Debug ./build_run.sh build` 仅构建）。

### C 项目编译

`c/middleware/` 下的子项目同样使用 CMake + `build_run.sh`，以 `stdf_os` 为例：

```bash
cd c/middleware/stdf_os
chmod +x build_run.sh
./build_run.sh
```

`stdf_os` 的运行不依赖硬件外设（按键由 pthread 模拟）；`stdf_mvc` 为纯软件演示。两者均无 `sudo` 需求。

### Python 项目运行

建议使用虚拟环境隔离依赖：

```bash
cd python/audio/ai_chat
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
python3 main.py
```

YOLO 示例的运行入口位于 `python/yolo/`，模型权重默认放在 `python/yolo/models/`（Hailo 权重 `yolov8n_h8l.hef` 需自行放置，仓库仅占位）。

### Camera 启用提示

摄像头相关示例（无论 Python / C++）首次运行前，请先在树莓派上启用摄像头：

```bash
sudo raspi-config
# Interface Options → Camera → Enable → 重启
```

---

## 📦 第三方 / 杂项

`misc/` 目录用于存放与示例配套的第三方二进制或配置，例如 `misc/mediamtx/`（用于配合 HTTP / WebSocket 视频流示例的轻量级媒体服务器）。

仓库根的 `.gitignore` 已默认排除 `build/`、`.venv/`、模型权重（`*.pt` / `*.onnx` / `*.hef` 等），请勿手动提交。

---

## 📚 参考资料

- [树莓派官方文档](https://www.raspberrypi.com/documentation/)
- [libcamera / rpicam-apps](https://libcamera.org/)
- [Ultralytics YOLOv8](https://docs.ultalytics.com/)
- [Hailo-8L 官方文档](https://hailo.ai/)
- [sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx)