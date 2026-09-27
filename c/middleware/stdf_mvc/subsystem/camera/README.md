# camera 子系统（纯数据层）

> 纯数据层：仅定义 subject ID 枚举与 payload 结构体（无任何业务逻辑）。后续扩展视频帧、云台、AI 检测等 camera 相关发布。

## subject 定义

| ID | 名称 | payload 大小 |
|---|---|---|
| `STDF_MVC_SUBJECT_CAMERA_FRAME_RAW` | `camera.frame_raw` | 0（占位） |
| `STDF_MVC_SUBJECT_CAMERA_FRAME_ENCODED` | `camera.frame_encoded` | 0（占位） |
| `STDF_MVC_SUBJECT_CAMERA_AI_RESULT` | `camera.ai_result` | 0（占位） |
| `STDF_MVC_SUBJECT_CAMERA_GIMBAL_STATE` | `camera.gimbal_state` | 0（占位） |

## 文件

- `std_mvc_subject_camera.h` —— subject ID 枚举
- `std_mvc_data_camera.h` —— payload 结构体（当前为空）
