/* stdf_define - 全局日志 / 断言 / 等级阈值。所有 stdf_mvc 模块依赖。 */

#ifndef __STDF_DEFINE_H__
#define __STDF_DEFINE_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 基础 STDF_LOG 是无前缀的 fprintf，供子系统/模块 LOG 宏拼 "[MVC] " 这类前缀；
   业务侧统一使用下面的 STDF_LOG_I / W / E / D 多等级宏。 */
#define STDF_LOG(fmt, ...)              fprintf(stderr, "[STDF]" fmt, ##__VA_ARGS__)

/* 多等级 LOG。输出格式：[STDF][L] <func> <msg>，带等级阈值编译期优化。 */
#define STDF_LOG_LEVEL_DEBUG            0
#define STDF_LOG_LEVEL_INFO             1
#define STDF_LOG_LEVEL_WARN             2
#define STDF_LOG_LEVEL_ERROR            3

#ifndef STDF_LOG_LEVEL
#define STDF_LOG_LEVEL                  STDF_LOG_LEVEL_INFO
#endif

#define STDF_LOG_D(fmt, ...)            do {                                                                    \
                                                if (STDF_LOG_LEVEL <= STDF_LOG_LEVEL_DEBUG) {                 \
                                                    fprintf(stderr, "[STDF][D] %s " fmt "\n", __func__,       \
                                                            ##__VA_ARGS__);                                  \
                                                }                                                             \
                                            } while (0)

#define STDF_LOG_I(fmt, ...)            do {                                                                    \
                                                if (STDF_LOG_LEVEL <= STDF_LOG_LEVEL_INFO) {                  \
                                                    fprintf(stderr, "[STDF][I] %s " fmt "\n", __func__,       \
                                                            ##__VA_ARGS__);                                  \
                                                }                                                             \
                                            } while (0)

#define STDF_LOG_W(fmt, ...)            do {                                                                    \
                                                if (STDF_LOG_LEVEL <= STDF_LOG_LEVEL_WARN) {                  \
                                                    fprintf(stderr, "[STDF][W] %s " fmt "\n", __func__,       \
                                                            ##__VA_ARGS__);                                  \
                                                }                                                             \
                                            } while (0)

#define STDF_LOG_E(fmt, ...)            do {                                                                    \
                                                if (STDF_LOG_LEVEL <= STDF_LOG_LEVEL_ERROR) {                 \
                                                    fprintf(stderr, "[STDF][E] %s " fmt "\n", __func__,       \
                                                            ##__VA_ARGS__);                                  \
                                                }                                                             \
                                            } while (0)

#define STDF_DUMP8(str, buf, cnt)       do { (void)(str); (void)(buf); (void)(cnt); } while (0)

#define STDF_ASSERT(cond)               do {                                                                    \
                                                if (!(cond)) {                                                 \
                                                    fprintf(stderr, "[STDF][ASSERT] %s line %d\n",             \
                                                            __func__, __LINE__);                              \
                                                    abort();                                                   \
                                                }                                                               \
                                            } while (0)

#ifdef __cplusplus
}
#endif

#endif /* __STDF_DEFINE_H__ */
