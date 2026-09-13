/* stdf_app_payload_video - 视频流域 payload */

#ifndef __STDF_APP_PAYLOAD_VIDEO_H__
#define __STDF_APP_PAYLOAD_VIDEO_H__

#include <stdint.h>

typedef enum {
    VIDEO_CODEC_H264 = 0,
    VIDEO_CODEC_H265,
} stdf_app_video_codec_t;

typedef enum {
    VIDEO_STATE_IDLE = 0,
    VIDEO_STATE_RECORDING,
    VIDEO_STATE_PAUSED,
    VIDEO_STATE_ERROR,
} stdf_app_video_state_t;

typedef enum {
    VIDEO_RES_720P  = 0,
    VIDEO_RES_1080P = 1,
    VIDEO_RES_4K    = 2,
} stdf_app_video_resolution_t;

typedef struct {
    void           *virt_addr;
    uint32_t        size_bytes;
    uint32_t        width;
    uint32_t        height;
    uint64_t        timestamp_ms;
} stdf_app_video_frame_raw_t;

typedef struct {
    void                       *data;
    uint32_t                    size_bytes;
    stdf_app_video_codec_t      codec;
    uint64_t                    timestamp_ms;
    uint8_t                     is_keyframe;
} stdf_app_video_frame_encoded_t;

typedef struct {
    stdf_app_video_state_t      state;
    uint64_t                    duration_ms;
    uint32_t                    file_index;
} stdf_app_video_record_t;

typedef struct {
    stdf_app_video_resolution_t res;
    uint32_t                    width;
    uint32_t                    height;
} stdf_app_video_resolution_info_t;

#endif
