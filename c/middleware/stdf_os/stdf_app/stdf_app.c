/* stdf_app - 业务子系统聚合 init */

#include "stdf_define.h"
#include "stdf_app.h"

#include "stdf_app_heartbeat.h"
#include "stdf_app_key.h"
#include "stdf_app_key_sim.h"

void stdf_app_init(void)
{
    stdf_app_key_init();
    stdf_app_key_sim_init();
    stdf_app_heartbeat_init();
}
