/* stdf_app_ai - AI 检测/跟踪 (订阅图像 → 发射检测/跟踪) */

#ifndef __STDF_APP_AI_H__
#define __STDF_APP_AI_H__

#ifdef __cplusplus
extern "C" {
#endif

int stdf_app_ai_init(void);
void stdf_app_ai_tick(void);

#ifdef __cplusplus
}
#endif

#endif
