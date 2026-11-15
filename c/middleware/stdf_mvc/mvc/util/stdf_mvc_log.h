/* stdf_mvc logging helpers. */
#ifndef __STDF_MVC_LOG_H__
#define __STDF_MVC_LOG_H__
#include <stdarg.h>
#include <stdio.h>
#define STDF_MVC_LOG_LEVEL_DEBUG 0
#define STDF_MVC_LOG_LEVEL_INFO 1
#define STDF_MVC_LOG_LEVEL_WARN 2
#define STDF_MVC_LOG_LEVEL_ERROR 3
#ifndef STDF_MVC_LOG_LEVEL
#define STDF_MVC_LOG_LEVEL STDF_MVC_LOG_LEVEL_INFO
#endif
static inline void stdf_mvc_log_write(int level, const char *tag,
                                      const char *function, const char *format, ...)
{
    if (level < STDF_MVC_LOG_LEVEL) {
        return;
    }
    va_list arguments;
    va_start(arguments, format);
    flockfile(stderr);
    fprintf(stderr, "[STDF_MVC][%s] %s %s ",
            level == 0 ? "D" : level == 1 ? "I" : level == 2 ? "W" : "E", tag, function);
    vfprintf(stderr, format, arguments);
    fputc('\n', stderr);
    funlockfile(stderr);
    va_end(arguments);
}
#define STDF_MVC_LOG_D(...) stdf_mvc_log_write(0, "", __func__, __VA_ARGS__)
#define STDF_MVC_LOG_I(...) stdf_mvc_log_write(1, "", __func__, __VA_ARGS__)
#define STDF_MVC_LOG_W(...) stdf_mvc_log_write(2, "", __func__, __VA_ARGS__)
#define STDF_MVC_LOG_E(...) stdf_mvc_log_write(3, "", __func__, __VA_ARGS__)
#define STDF_MVC_LOG_D_TAG(tag, ...) stdf_mvc_log_write(0, tag, __func__, __VA_ARGS__)
#define STDF_MVC_LOG_I_TAG(tag, ...) stdf_mvc_log_write(1, tag, __func__, __VA_ARGS__)
#define STDF_MVC_LOG_W_TAG(tag, ...) stdf_mvc_log_write(2, tag, __func__, __VA_ARGS__)
#define STDF_MVC_LOG_E_TAG(tag, ...) stdf_mvc_log_write(3, tag, __func__, __VA_ARGS__)
#define STDF_MVC_LOGI(...) STDF_MVC_LOG_I(__VA_ARGS__)
#define STDF_MVC_LOGE(...) STDF_MVC_LOG_E(__VA_ARGS__)
#endif
