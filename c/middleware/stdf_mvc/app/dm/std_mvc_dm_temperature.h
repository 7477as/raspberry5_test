/* std_mvc_dm_temperature - 树莓派 CPU 温度发布者 */

#ifndef __STDF_MVC_DM_TEMPERATURE_H__
#define __STDF_MVC_DM_TEMPERATURE_H__

#include "subsystem/dm/std_mvc_data_dm.h"

#ifdef __cplusplus
extern "C" {
#endif

int  std_mvc_dm_temperature_init(void);
void std_mvc_dm_temperature_tick(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_MVC_DM_TEMPERATURE_H__ */
