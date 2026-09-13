/* stdf_mvc_payload_user - 用户设置域 payload */

#ifndef __STDF_MVC_PAYLOAD_USER_H__
#define __STDF_MVC_PAYLOAD_USER_H__

#include <stdint.h>

typedef enum {
    STDF_MVC_PAYLOAD_USER_MODE_PHOTO   = 0,
    STDF_MVC_PAYLOAD_USER_MODE_VIDEO   = 1,
    STDF_MVC_PAYLOAD_USER_MODE_LIVE    = 2,
    STDF_MVC_PAYLOAD_USER_MODE_PLAYBACK= 3,
    STDF_MVC_PAYLOAD_USER_MODE_SETUP   = 4,
} stdf_mvc_payload_user_mode_t;

typedef struct {
    stdf_mvc_payload_user_mode_t     mode;
} stdf_mvc_payload_user_mode_event_t;

typedef struct {
    char                     key[32];
    int32_t                  int_value;
    char                     str_value[32];
} stdf_mvc_payload_user_setting_t;

typedef struct {
    char                     profile_name[32];
} stdf_mvc_payload_user_profile_t;

typedef struct {
    char                     lang_code[8];
} stdf_mvc_payload_language_t;

#endif
