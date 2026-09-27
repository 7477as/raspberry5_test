# stdf_mvc 演示

> 发布订阅中间件，v4 简化版：per-subject cache publisher-中立、4 个公开 API、read 2 参数、subject 名称与 size 表统一从 X-macro 派生。

## 功能简介

`stdf_mvc` 是一个**发布订阅 (pub/sub) 中间件**，专为资源受限的嵌入式 MCU 设计：

- **v4 简化**：`read(id, data)` 仅 2 参数；per-subject cache 每 subject 单槽（不按 publisher 分槽）；4 个公开 API 第 4 参数统一命名为 `data`
- **按实际占用分配 cache**：编译期 X-macro 求和，只分配真正用到的字节
- **publisher 过滤**：发布者可为 `STD_MVC_LOCAL` / `STD_MVC_REMOTE` / `STD_MVC_ANY`（ANY 接收所有）
- **pub_type**：当前仅 `STD_MVC_PUB_ASYNC`，`STD_MVC_PUB_SYNC` 保留待实现
- **4 子系统**：dm（数据管理）、net（网络）、camera（视频/AI）、ui（界面）
- **静态内存**：零默认堆分配，节点池 + 环形队列全部静态

### 关键特性

| 特性 | 说明 |
| :--- | :--- |
| 内存模型 | 静态 slot 池（64 节点）+ ring queue（256 槽）+ per-subject cache |
| cache 大小 | 编译期 X-macro 实测求和（demo: 16B = sizeof(temperature_sample)） |
| 线程安全 | 全局互斥；订阅/退订持锁，发射不持锁 |
| 跨线程 | async worker thread（pthread + condvar） |
| 平台抽象 | `stdf_mvc_core_port.h` 接口 + linux / melis 两份实现 |
| Payload | 业务 struct 直传，零拷贝 |

## 依赖 / 环境

- 硬件：树莓派 5（验证环境）
- 系统：Raspberry Pi OS Bookworm 64-bit
- 编译器：gcc + cmake >= 3.16
- 第三方库：**无**（仅 pthread，标准库自带）
- 可选 SDL2：`sudo apt install libsdl2-dev`（安装后自动编译 SDL2 UI 模块）

## 目录结构

```
stdf_mvc/
├── CMakeLists.txt
├── build_run.sh
├── main.c
├── stdf_mvc_api.h/.c            # 公开 4 接口（v4 简化签名）
│
├── core/                        # 框架核心
│   ├── stdf_mvc_core.h/.c      # init/deinit/控制
│   ├── stdf_mvc_core_types.h   # publisher/pub_type/slot_fn（含 STD_MVC_ANY）
│   ├── stdf_mvc_core_config.h
│   ├── stdf_mvc_core_meta.h/.c # payload_size X-macro 表
│   ├── stdf_mvc_core_slot.h
│   ├── stdf_mvc_core_subject.h/.c
│   ├── stdf_mvc_core_pool.h/.c
│   ├── stdf_mvc_core_data.h/.c     # per-subject cache（publisher-中立）
│   ├── stdf_mvc_core_dispatch.h/.c
│   ├── stdf_mvc_core_async.h
│   ├── stdf_mvc_core_async_linux.c
│   ├── stdf_mvc_core_port.h/.c     # 平台无关默认（mem_alloc/free）
│   ├── stdf_mvc_core_port_linux.c  # pthread critical section
│   └── stdf_mvc_core_port_melis.c   # 全志 melis 占位
│
├── util/
│   └── std_mvc_log.h            # STD_MVC_LOG_I/W/E/D + STD_MVC_ASSERT
│
├── subsystem/                    # 纯数据（无业务逻辑）
│   ├── std_mvc_subsystems.h     # X-macro (name/size/sum)，通过 CMake -include 注入
│   ├── dm/
│   │   ├── std_mvc_subject_dm.h    # subject ID 枚举
│   │   └── std_mvc_data_dm.h       # payload 结构体（当前仅温度）
│   ├── net/                     # 同上（占位）
│   ├── camera/                  # 同上（占位）
│   └── ui/                     # 同上（占位）
│
└── app/                        # 业务层
    ├── std_mvc_apps.h/.c        # 聚合入口（app_init/tick/deinit）
    ├── dm/std_mvc_dm_temperature.h/.c  # 温度发布者
    ├── net/std_mvc_net.h/.c           # 网络（占位）
    ├── camera/std_mvc_camera.h/.c     # 视频/AI（占位）
    └── ui/
        ├── std_mvc_ui_print.h/.c     # 打印订阅者
        └── std_mvc_ui_sdl.h/.c        # SDL2 图形 UI（可选，libsdl2-dev）
```

## 分层约束

| 层 | 允许 include | 禁止 |
|---|---|---|
| `core/**` | 标准库、自家私有头 | `subsystem/**`、`app/**` |
| `subsystem/**` | `core/stdf_mvc_core_types.h`、同层数据头、标准库 | `stdf_mvc_api.h`、`app/**` |
| `app/**` | `stdf_mvc_api.h`、`subsystem/<x>/std_mvc_data_<x>.h`、`util/std_mvc_log.h` | `core/**` 私有头 |
| `main.c` | `stdf_mvc_api.h`、`app/std_mvc_apps.h` | `core/**`、`subsystem/**` |

## 构建与运行

```bash
cd c/middleware/stdf_mvc
chmod +x build_run.sh
./build_run.sh          # 配置 + 构建 + 运行（默认）
./build_run.sh build    # 仅构建
./build_run.sh clean    # 清理 build 目录
./build_run.sh rebuild  # clean + build
NORUN=1 ./build_run.sh  # 仅构建不运行
BUILD_TYPE=Debug ./build_run.sh
```

安装 SDL2 以启用图形 UI：

```bash
sudo apt install libsdl2-dev
./build_run.sh
# 弹出 800x480 窗口，温度条随采样更新
# Ctrl+C 或 Esc 退出
```

## 关键参数 / 配置

| 宏 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `STD_MVC_POOL_SIZE` | `64` | 静态 slot 池容量 |
| `STD_MVC_ASYNC_QUEUE_DEPTH` | `256` | 异步 ring queue 深度 |
| `STD_MVC_SUB_CHANGE_LOG` | `1` | 订阅/退订事件 debug 日志 |
| `STD_MVC_LOG_LEVEL` | `INFO` | `DEBUG`/`INFO`/`WARN`/`ERROR` |
| `CPU_TEMP_PATH` | `/sys/class/thermal/thermal_zone0/temp` | CPU 温度读取路径（Pi 5 SoC） |
| `TEMPERATURE_PUBLISH_DELTA_C` | `1.0f` | 温度发布阈值（℃）；差值 ≥ 此值才再次发布 |
| `TEMPERATURE_SAMPLE_PERIOD_MS` | `1000u` | 温度采样周期（每 tick 100ms） |

## 公开 API（v4 简化版）

```c
#include "stdf_mvc_api.h"

int stdf_mvc_api_subscribe(std_mvc_publisher_t   publisher,
                            stdf_mvc_subject_id_t id,
                            stdf_mvc_slot_fn_t   fn);

int stdf_mvc_api_unsubscribe(std_mvc_publisher_t   publisher,
                              stdf_mvc_subject_id_t id,
                              stdf_mvc_slot_fn_t   fn);

int stdf_mvc_api_publish(std_mvc_publisher_t   publisher,
                          stdf_mvc_subject_id_t id,
                          const void          *data,
                          std_mvc_pub_type_t   pub_type);

int stdf_mvc_api_read(stdf_mvc_subject_id_t id, void *data);
```

> v4 设计：subscribe/unsubscribe 三参数；slot 回调的第一个参数是 `subject_id`（v5 起替代原 `user_data`，让一个 fn 能处理同一 publisher 的多个 subject）；publish 第 4 参数是 payload；read 是输出 buffer（仅 2 参数，由 subject ID 编译期决定复制多少字节，无需 buf_size）。

### slot 回调签名

```c
typedef void (*stdf_mvc_slot_fn_t)(stdf_mvc_subject_id_t  subject_id,
                                    std_mvc_publisher_t    publisher,
                                    const void           *data);
```

### publisher 过滤

| 常量 | 值 | 含义 |
| :--- | :--- | :--- |
| `STD_MVC_LOCAL` | `0` | 仅接收同 MCU 的 publish |
| `STD_MVC_REMOTE` | `1` | 仅接收网络代理的 publish |
| `STD_MVC_ANY` | `0x7F` | 接收所有 publisher 的 publish |

## 平台移植

### Linux（验证环境）

`core/stdf_mvc_core_async_linux.c` + `core/stdf_mvc_core_port_linux.c`（pthread）。

### 全志 melis（占位）

`core/stdf_mvc_core_port_melis.c` 提供空实现；异步改用 melis `osMessageQ` 或主循环轮询。

## 运行效果

```
[STD_MVC][I] stdf_mvc_core_init stdf_mvc_core init ok, subjects=303 pool=64 cache=16 bytes
[STD_MVC][I] [APPS] std_mvc_apps_init init
[STD_MVC][I] [DM-TEMP] std_mvc_dm_temperature_init (首次 CPU 温度已发布)
[STD_MVC][I] [UI] std_mvc_ui_print_init print init
[STD_MVC][I] [UI] on_temperature LOCAL temp: 47.34 C @ 1704000 ms
[STD_MVC][I] main running... (Ctrl+C to exit)
[STD_MVC][I] main === dump @ ... ms ===
[STD_MVC][I] stdf_mvc_core_dispatch_dump === stdf_mvc subjects ===
[STD_MVC][I] stdf_mvc_core_dispatch_dump   [1] dm.temperature: 1 subs
[STD_MVC][I] stdf_mvc_core_dispatch_dump === pool: 1/64 used ===
[STD_MVC][I] main shutting down...
[STD_MVC][I] stdf_mvc_core_deinit stdf_mvc_core deinit ok
```

> 温度源：`/sys/class/thermal/thermal_zone0/temp`（Pi 5 SoC 温度）。首次 `init` 时主动 publish 一次，之后只有与上次发布的差值 ≥ 1°C 才再次 publish，避免热噪声导致刷屏。

## 参考资料

- STDF 编码规范（项目内 `.cursor/rules/10-c.mdc`）
