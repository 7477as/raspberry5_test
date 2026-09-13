/* stdf_app_payload_ai - AI 视觉域 payload */

#ifndef __STDF_APP_PAYLOAD_AI_H__
#define __STDF_APP_PAYLOAD_AI_H__

#include <stdint.h>

typedef enum {
    AI_OBJ_PERSON    = 0,
    AI_OBJ_FACE      = 1,
    AI_OBJ_CAR       = 2,
    AI_OBJ_PET       = 3,
    AI_OBJ_OTHER     = 0xFF,
} stdf_app_ai_object_class_t;

typedef enum {
    AI_TRACK_LOST    = 0,
    AI_TRACK_SEARCH  = 1,
    AI_TRACK_LOCKED  = 2,
} stdf_app_ai_tracking_state_t;

typedef struct {
    stdf_app_ai_object_class_t  object_class;
    uint32_t                    track_id;
    float                       confidence;
    int32_t                     bbox_x;
    int32_t                     bbox_y;
    int32_t                     bbox_w;
    int32_t                     bbox_h;
    uint64_t                    timestamp_ms;
} stdf_app_ai_detection_t;

typedef struct {
    stdf_app_ai_tracking_state_t state;
    uint32_t                     track_id;
    stdf_app_ai_object_class_t   object_class;
} stdf_app_ai_tracking_t;

typedef struct {
    char        name[32];
    uint32_t    face_id;
    float       confidence;
} stdf_app_ai_face_t;

typedef struct {
    char        gesture_name[32];
    float       confidence;
} stdf_app_ai_gesture_t;

typedef struct {
    char        scene_name[32];
} stdf_app_ai_scene_t;

#endif
