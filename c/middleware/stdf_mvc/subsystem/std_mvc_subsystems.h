/* std_mvc_subsystems - 聚合 4 子系统的 payload 大小 / name / sum 表 */
/* 通过 CMake -include 注入到每个 .c 顶部，无 include guard */

#include "core/stdf_mvc_core_subject.h"
#include "dm/std_mvc_data_dm.h"
#include "net/std_mvc_data_net.h"
#include "camera/std_mvc_data_camera.h"
#include "ui/std_mvc_data_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* subject 定义 X-list：(id, payload_size_bytes, name_str) */
/* 必须与 core/stdf_mvc_core_subject.h 中 enum 顺序一致 */
#define STDF_MVC_SUBJECT_DEF(X)                                                          \
    X(0,   0,                                              "")                          \
    X(1,   sizeof(std_mvc_dm_temperature_t),        "dm.temperature")           \
    X(2,   0,                                              "dm.humidity")              \
    X(3,   0,                                              "dm.battery")               \
    X(4,   0,                                              "dm.storage")               \
    X(5,   0,                                              "dm.network")               \
    X(100, 0,                                              "net.wifi_state")            \
    X(101, 0,                                              "net.eth_state")             \
    X(102, 0,                                              "net.mqtt_state")            \
    X(103, 0,                                              "net.ntp_sync")              \
    X(200, 0,                                              "camera.frame_raw")          \
    X(201, 0,                                              "camera.frame_encoded")      \
    X(202, 0,                                              "camera.ai_result")          \
    X(203, 0,                                              "camera.gimbal_state")       \
    X(300, 0,                                              "ui.button_pressed")         \
    X(301, 0,                                              "ui.screen_touch")           \
    X(302, 0,                                              "ui.display_update")

/* X 应用器：分别用于生成 size 表 / name 表 / 总字节数 sum 表达式 */
#define STD_MVC_SIZE_X(id, sz, name)    [id] = sz,
#define STD_MVC_NAME_X(id, sz, name)    [id] = name,
#define STD_MVC_SUM_X(id, sz, name)     + (sz)

#define STDF_MVC_PAYLOAD_SIZES            STDF_MVC_SUBJECT_DEF(STD_MVC_SIZE_X)
#define STDF_MVC_SUBJECT_NAMES            STDF_MVC_SUBJECT_DEF(STD_MVC_NAME_X)
#define STD_MVC_TOTAL_CACHE_BYTES         ((0u) STDF_MVC_SUBJECT_DEF(STD_MVC_SUM_X))
