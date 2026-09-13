/* stdf_app_video - 视频录制 (订阅 AI → 发射视频帧/录制状态) */

#ifndef __STDF_APP_VIDEO_H__
#define __STDF_APP_VIDEO_H__

#ifdef __cplusplus
extern "C" {
#endif

int  stdf_app_video_init(void);
void stdf_app_video_tick(void);

#ifdef __cplusplus
}
#endif

#endif
