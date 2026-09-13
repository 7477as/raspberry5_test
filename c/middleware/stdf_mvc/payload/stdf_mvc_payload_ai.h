/* stdf_mvc_payload_ai - AI 视觉域 payload */

#ifndef __STDF_MVC_PAYLOAD_AI_H__
#define __STDF_MVC_PAYLOAD_AI_H__

#include <stdint.h>

typedef enum {
    STDF_MVC_PAYLOAD_AI_OBJ_PERSON    = 0,
    STDF_MVC_PAYLOAD_AI_OBJ_FACE      = 1,
    STDF_MVC_PAYLOAD_AI_OBJ_CAR       = 2,
    STDF_MVC_PAYLOAD_AI_OBJ_PET       = 3,
    STDF_MVC_PAYLOAD_AI_OBJ_OTHER     = 0xFF,
} stdf_mvc_payload_ai_object_class_t;

typedef enum {
    STDF_MVC_PAYLOAD_AI_TRACK_LOST    = 0,
    STDF_MVC_PAYLOAD_AI_TRACK_SEARCH  = 1,
    STDF_MVC_PAYLOAD_AI_TRACK_LOCKED  = 2,
} stdf_mvc_payload_ai_tracking_state_t;

typedef struct {
    stdf_mvc_payload_ai_object_class_t  object_class;
    uint32_t                    track_id;
    float                       confidence;
    int32_t                     bbox_x;
    int32_t                     bbox_y;
    int32_t                     bbox_w;
    int32_t                     bbox_h;
    uint64_t                    timestamp_ms;
} stdf_mvc_payload_ai_detection_t;

typedef struct {
    stdf_mvc_payload_ai_tracking_state_t state;
    uint32_t                     track_id;
    stdf_mvc_payload_ai_object_class_t   object_class;
} stdf_mvc_payload_ai_tracking_t;

typedef struct {
    char        name[32];
    uint32_t    face_id;
    float       confidence;
} stdf_mvc_payload_ai_face_t;

typedef struct {
    char        gesture_name[32];
    float       confidence;
} stdf_mvc_payload_ai_gesture_t;

typedef struct {
    char        scene_name[32];
} stdf_mvc_payload_ai_scene_t;

#endif
