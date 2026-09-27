/* std_mvc_log - 全局日志 / 断言宏 */

#ifndef __STD_MVC_LOG_H__
#define __STD_MVC_LOG_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STD_MVC_LOG(fmt, ...)              fprintf(stderr, "[STD_MVC] " fmt "\n", ##__VA_ARGS__)

#define STD_MVC_LOG_LEVEL_DEBUG            0
#define STD_MVC_LOG_LEVEL_INFO             1
#define STD_MVC_LOG_LEVEL_WARN             2
#define STD_MVC_LOG_LEVEL_ERROR            3

#ifndef STD_MVC_LOG_LEVEL
#define STD_MVC_LOG_LEVEL                  STD_MVC_LOG_LEVEL_INFO
#endif

#define STD_MVC_LOG_D(fmt, ...)            do {                                                                    \
                                                if (STD_MVC_LOG_LEVEL <= STD_MVC_LOG_LEVEL_DEBUG) {            \
                                                    fprintf(stderr, "[STD_MVC][D] %s " fmt "\n", __func__,     \
                                                            ##__VA_ARGS__);                                    \
                                                }                                                               \
                                            } while (0)

#define STD_MVC_LOG_I(fmt, ...)            do {                                                                    \
                                                if (STD_MVC_LOG_LEVEL <= STD_MVC_LOG_LEVEL_INFO) {             \
                                                    fprintf(stderr, "[STD_MVC][I] %s " fmt "\n", __func__,     \
                                                            ##__VA_ARGS__);                                    \
                                                }                                                               \
                                            } while (0)

#define STD_MVC_LOG_W(fmt, ...)            do {                                                                    \
                                                if (STD_MVC_LOG_LEVEL <= STD_MVC_LOG_LEVEL_WARN) {             \
                                                    fprintf(stderr, "[STD_MVC][W] %s " fmt "\n", __func__,     \
                                                            ##__VA_ARGS__);                                    \
                                                }                                                               \
                                            } while (0)

#define STD_MVC_LOG_E(fmt, ...)            do {                                                                    \
                                                if (STD_MVC_LOG_LEVEL <= STD_MVC_LOG_LEVEL_ERROR) {            \
                                                    fprintf(stderr, "[STD_MVC][E] %s " fmt "\n", __func__,     \
                                                            ##__VA_ARGS__);                                    \
                                                }                                                               \
                                            } while (0)

#define STD_MVC_LOG_D_TAG(tag, fmt, ...)   do {                                                                    \
                                                if (STD_MVC_LOG_LEVEL <= STD_MVC_LOG_LEVEL_DEBUG) {            \
                                                    fprintf(stderr, "[STD_MVC][D] %s %s " fmt "\n", (tag),     \
                                                            __func__, ##__VA_ARGS__);                          \
                                                }                                                               \
                                            } while (0)

#define STD_MVC_LOG_I_TAG(tag, fmt, ...)   do {                                                                    \
                                                if (STD_MVC_LOG_LEVEL <= STD_MVC_LOG_LEVEL_INFO) {             \
                                                    fprintf(stderr, "[STD_MVC][I] %s %s " fmt "\n", (tag),     \
                                                            __func__, ##__VA_ARGS__);                          \
                                                }                                                               \
                                            } while (0)

#define STD_MVC_LOG_W_TAG(tag, fmt, ...)   do {                                                                    \
                                                if (STD_MVC_LOG_LEVEL <= STD_MVC_LOG_LEVEL_WARN) {             \
                                                    fprintf(stderr, "[STD_MVC][W] %s %s " fmt "\n", (tag),     \
                                                            __func__, ##__VA_ARGS__);                          \
                                                }                                                               \
                                            } while (0)

#define STD_MVC_LOG_E_TAG(tag, fmt, ...)   do {                                                                    \
                                                if (STD_MVC_LOG_LEVEL <= STD_MVC_LOG_LEVEL_ERROR) {            \
                                                    fprintf(stderr, "[STD_MVC][E] %s %s " fmt "\n", (tag),     \
                                                            __func__, ##__VA_ARGS__);                          \
                                                }                                                               \
                                            } while (0)

#define STD_MVC_DUMP8(str, buf, cnt)       do { (void)(str); (void)(buf); (void)(cnt); } while (0)

#define STD_MVC_ASSERT(cond)               do {                                                                    \
                                                if (!(cond)) {                                                   \
                                                    fprintf(stderr, "[STD_MVC][ASSERT] %s line %d\n",           \
                                                            __func__, __LINE__);                                \
                                                    abort();                                                     \
                                                }                                                               \
                                            } while (0)

#ifdef __cplusplus
}
#endif

#endif /* __STD_MVC_LOG_H__ */
