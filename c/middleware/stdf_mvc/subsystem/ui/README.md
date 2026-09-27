# ui 子系统（纯数据层）

> 纯数据层：仅定义 subject ID 枚举与 payload 结构体（无任何业务逻辑）。

## subject 定义

| ID | 名称 | payload 大小 |
|---|---|---|
| `STDF_MVC_SUBJECT_UI_BUTTON_PRESSED` | `ui.button_pressed` | 0（占位） |
| `STDF_MVC_SUBJECT_UI_SCREEN_TOUCH` | `ui.screen_touch` | 0（占位） |
| `STDF_MVC_SUBJECT_UI_DISPLAY_UPDATE` | `ui.display_update` | 0（占位） |

## 文件

- `std_mvc_subject_ui.h` —— subject ID 枚举
- `std_mvc_data_ui.h` —— payload 结构体（当前为空）
