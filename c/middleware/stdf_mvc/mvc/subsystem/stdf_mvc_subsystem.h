/* stdf_mvc subsystem subject and payload aggregation. */
#ifndef __STDF_MVC_SUBSYSTEM_H__
#define __STDF_MVC_SUBSYSTEM_H__

#include "dm/stdf_mvc_subject_dm.h"
#include "dm/stdf_mvc_data_dm.h"
#include "net/stdf_mvc_subject_net.h"
#include "net/stdf_mvc_data_net.h"
#include "camera/stdf_mvc_subject_camera.h"
#include "camera/stdf_mvc_data_camera.h"
#include "ui/stdf_mvc_subject_ui.h"
#include "ui/stdf_mvc_data_ui.h"

#define STDF_MVC_SUBJECT_LIST(X) \
    STDF_MVC_DM_SUBJECT_LIST(X) \
    STDF_MVC_NET_SUBJECT_LIST(X) \
    STDF_MVC_CAMERA_SUBJECT_LIST(X) \
    STDF_MVC_UI_SUBJECT_LIST(X)

#define STDF_MVC_DATA_LIST(X) \
    STDF_MVC_DM_DATA_LIST(X) \
    STDF_MVC_NET_DATA_LIST(X) \
    STDF_MVC_CAMERA_DATA_LIST(X) \
    STDF_MVC_UI_DATA_LIST(X)

#endif
