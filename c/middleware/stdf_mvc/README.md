# stdf_mvc 演示

> 云台相机场景的发布订阅中间件，覆盖 60 个业务事件，单例、跨线程、平台可移植。

## 功能简介

`stdf_mvc` 是一个**发布订阅 (pub/sub) 中间件**，按 STDF 编码规范实现，专为云台相机的 60+ 业务事件设计：

- **视频流域** (7)：原始帧、编码帧、录制状态、录制时长、分辨率、码率、夜视
- **AI 视觉** (6)：检测结果、跟踪状态、目标切换、人脸识别、手势、场景
- **云台控制** (7)：当前姿态、目标姿态、模式、运动、过载、摇杆响应
- **IMU 姿态** (5)：加速度、陀螺仪、四元数、欧拉角、温度
- **传感器** (6)：电池电压/电量/充电/健康/温度/湿度
- **用户输入** (5)：按键、触摸、旋钮、遥控、语音
- **网络** (8)：WiFi/BLE/RTMP/MQTT/NTP/RTSP 状态
- **存储** (5)：SD 卡状态、剩余空间、拍照、文件传输、文件列表
- **系统** (6)：启动/关机/固件更新/OTA/错误/日志
- **设置** (4)：用户模式、设置项、配置、语言

### 关键特性

| 特性 | 说明 |
| :--- | :--- |
| 内存模型 | 静态节点池（4096 节点），零默认分配 |
| 线程安全 | 全局互斥；订阅/退订持锁，发射不持锁 |
| 重入安全 | Slot 内可 unsubscribe / re-subscribe（缓存 next） |
| 跨线程 | `emit_async` 走 port 层环形队列 + worker 线程 |
| 调试 | `dump_subjects()` 打印所有订阅状态 + emit 计数 |
| 平台抽象 | `stdf_mvc_port.h` 接口 + linux / melis 两份实现 (mem/critical/tick/post_async；日志由 `stdf_define.h` 统一处理) |
| Payload | 8B union（值类型）+ `void *` 借用（大 payload 借用语义） |
| 业务解耦 | Subject / payload / 业务模块 三层分离，框架零业务耦合 |

## 依赖 / 环境

- 硬件：树莓派 5（验证环境）
- 系统：Raspberry Pi OS Bookworm 64-bit
- 编译器：gcc + cmake ≥ 3.16
- 第三方库：**无**（仅 pthread，标准库自带）

## 目录结构

```
stdf_mvc/
├── CMakeLists.txt                # 构建
├── build_run.sh                  # run/build/clean/rebuild
├── main.c                        # 入口
│
├── mvc/                          # ── 框架核心 (零业务耦合)
│   ├── stdf_mvc.h                # 聚合门面
│   ├── stdf_mvc.c                # 聚合 init / deinit
│   ├── stdf_mvc_config.h         # 编译期配置
│   ├── stdf_mvc_port.h           # 平台抽象接口
│   ├── stdf_mvc_port.c           # 框架对 ops 的访问层
│   ├── stdf_mvc_port_linux.c     # 树莓派/Linux 实现 (pthread)
│   ├── stdf_mvc_port_melis.c     # 全志 F136 melis 占位
│   ├── stdf_mvc_signal.h         # 8B union + ptr 借用约定
│   ├── stdf_mvc_slot.h           # slot 节点定义
│   ├── stdf_mvc_subject.h        # 框架侧 X-macro 入口
│   ├── stdf_mvc_subject.c        # name 表 (调试用)
│   ├── stdf_mvc_pool.h/.c        # 静态节点池
│   ├── stdf_mvc_emit.h/.c        # 同步发射 + subscribe/unsubscribe
│   ├── stdf_mvc_async.h/.c       # 异步发射 (pthread + condvar)
│   └── stdf_mvc_async_melis.c    # 异步发射 (melis 占位)
│
├── stdf_define.h                 # ── 全局 STDF_LOG_I/W/E/D + STDF_ASSERT (LOG 风格参考 stdf_os)
│
├── subject/                      # ── 业务 subject id 定义 (相机60 个事件)
│   ├── stdf_mvc_subjects.h       # 拼装主入口 (含 STDF_MVC_SUBJECT_LIST 宏)
│   └── stdf_mvc_subject_<domain>.h   # 10 个业务域 (ai/video/gimbal/...)
│
├── payload/                      # ── 业务 payload 数据结构
│   ├── stdf_mvc_payloads.h       # 主入口
│   └── stdf_mvc_payload_<domain>.h   # 10 个业务域
│
└── app/                          # ── 业务子系统 (相机业务实现)
    ├── stdf_app.h                # 业务聚合
    ├── stdf_app.c                # stdf_app_init + tick
    ├── hal_dummy/                # 模拟硬件抽象 (无真硬件)
    ├── input_mock/               # 模拟按键 / 旋钮输入
    ├── imu/                      # IMU 数据源 (从 hal → emit)
    ├── battery/                  # 电池监控 (含低电告警)
    ├── network/                  # 网络状态
    ├── storage/                  # SD 卡 / 文件管理
    ├── ai/                       # AI 检测/跟踪模拟
    ├── gimbal/                   # 云台 PID 控制
    ├── video/                    # 视频录制
    └── indicator/                # LED / 蜂鸣器指示器 (订阅)
```

## 构建与运行

```bash
cd c/middleware/stdf_mvc
chmod +x build_run.sh
./build_run.sh          # 配置 + 构建 + 运行 (默认)
./build_run.sh build    # 仅构建
./build_run.sh clean    # 清理 build 目录
./build_run.sh rebuild  # clean + build
NORUN=1 ./build_run.sh  # 仅构建不运行
BUILD_TYPE=Debug ./build_run.sh
```

## 关键参数 / 配置

### `stdf_mvc_config.h`

| 宏 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `STDF_MVC_POOL_SIZE` | `4096` | 静态节点池容量 = subject 数 × 平均订阅者 + 预留 |
| `STDF_MVC_ENABLE_ASYNC` | `1` | 启用异步发射 |
| `STDF_MVC_SUB_CHANGE_LOG` | `1` | 订阅/退订事件日志 |
| `STDF_MVC_EMIT_COUNT` | `1` | 每个 subject 的 emit 计数器 |
| `STDF_MVC_ASYNC_QUEUE_DEPTH` | `256` | 异步环形队列深度 |

### 关键 API

```c
#include "stdf_mvc.h"

int  stdf_mvc_init(void);
void stdf_mvc_deinit(void);

int  stdf_mvc_subject_subscribe(stdf_mvc_subject_id_t      id,
                                stdf_mvc_slot_fn_t         fn,
                                void                      *user_data);
int  stdf_mvc_subject_unsubscribe(stdf_mvc_subject_id_t    id,
                                  stdf_mvc_slot_fn_t       fn,
                                  void                    *user_data);

int  stdf_mvc_subject_emit(stdf_mvc_subject_id_t          id,
                           const stdf_mvc_signal_data_t  *data);
int  stdf_mvc_subject_emit_async(stdf_mvc_subject_id_t    id,
                                 const stdf_mvc_signal_data_t *data);

uint16_t stdf_mvc_subject_get_subscriber_count(stdf_mvc_subject_id_t id);
uint32_t stdf_mvc_get_pool_used(void);
uint32_t stdf_mvc_get_pool_free(void);
void     stdf_mvc_dump_subjects(void);
```

### Payload 约定

```c
typedef union {
    void    *ptr;    /* 大 payload: 业务方拥有, emit 期间有效 */
    int      i;
    uint32_t u32;
    uint64_t u64;
    float    f;
    double   d;
    uint8_t  bytes[8];
} stdf_mvc_signal_data_t;

/* 借用语义: ptr 指向的内存由 emit 方拥有, slot 调用期间必须有效 */
/* emit_async: 框架会拷贝 union 8B, ptr 必须指向全局或长生命周期内存 */
```

## 平台移植

### 树莓派 (Linux)

当前验证环境。链接 `stdf_mvc_port_linux.c` + `stdf_mvc_async.c`：
- 互斥锁：`pthread_mutex`
- 异步队列：`pthread` + condvar + 环形队列
- tick 源：`clock_gettime(CLOCK_MONOTONIC)`

### 全志 F136 melis

替换为 `stdf_mvc_port_melis.c` + `stdf_mvc_async_melis.c`：
- 互斥锁：用 `enter_critical` / `exit_critical` 嵌套计数器（建议对接 RTOS 调度锁）
- 异步队列：melis `osMessageQ` 或主循环 `stdf_mvc_async_melis_poll()` 轮询
- tick 源：`hal_sys_timer_get()`

**移植步骤**：
1. CMakeLists.txt 里把 `stdf_mvc_port_linux.c` 换成 `stdf_mvc_port_melis.c`
2. CMakeLists.txt 里把 `stdf_mvc_async.c` 换成 `stdf_mvc_async_melis.c`
3. 业务模块 `stdf_app_*.c` 链接对应的 HAL 实现（melis 上链接真实驱动而非 `stdf_app_hal_dummy.c`）
4. 业务代码 **不动**

## 已知问题 / 注意事项

- `subject/stdf_mvc_subjects.h` 通过 CMake `-include stdf_mvc_subjects.h` 注入业务 subject 列表；这是为了让框架核心 `.c` 文件知道 `STDF_MVC_SUBJECT_COUNT`。其他工程复用时需在 CMake 中同步加 `-include`。
- 异步发射 (`emit_async`) 满队列策略：**丢弃最新**，并返回 `-3`。业务方如需不丢请改 port 层（增大 `STDF_MVC_ASYNC_QUEUE_DEPTH` 或换阻塞策略）。
- Slot 内可 `unsubscribe` 自身 / 其他 slot（链表安全），但不可 `unsubscribe` 哨兵节点（sentinel 不会被匹配）。
- `stdf_mvc_dump_subjects()` 只打印 `count > 0` 的 subject；排查时调 `stdf_mvc_get_pool_used()` 看池用量。
- Linux 平台用 `pthread_mutex` 做全局临界区；高频 emit 时锁竞争低（emit 期间不持锁）。melis 上若需要更高吞吐，可改为无锁队列 + 原子头指针。

## 运行效果

启动后每 5 秒打印一次 `dump_subjects()` 输出，类似：

```
[STDF][I] stdf_mvc_init stdf_mvc init ok, subjects=60 pool=4096
[STDF][I] main running... (Ctrl+C to exit)
[STDF][I] on_button [BTN] id=0 press
[STDF][I] on_record_state [LED] RED ON  (recording, file=1)
[STDF][I] on_tracking_state [LED] GREEN ON  (tracking id=124)
[STDF][I] main === dump @ 5003 ms ===
[STDF][I] stdf_mvc_emit_dump_subjects === stdf_mvc subjects ===
[STDF][I] stdf_mvc_emit_dump_subjects   [3] video.record_state: 1 subs
[STDF][I] stdf_mvc_emit_dump_subjects   [8] ai.detection_result: 1 subs
[STDF][I] stdf_mvc_emit_dump_subjects   [9] ai.tracking_state: 2 subs
[STDF][I] stdf_mvc_emit_dump_subjects   [32] button.pressed: 2 subs
[STDF][I] stdf_mvc_emit_dump_subjects === pool: 70/4096 used ===
[STDF][I] main pool: 70/4096 used
```

## 参考资料

- STDF 编码规范（项目内 `.cursor/rules/15-c-stdf.mdc`）
- 相机数据域设计（项目内）
