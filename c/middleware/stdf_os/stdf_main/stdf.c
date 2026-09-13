/* stdf - 顶层聚合入口实现 */

#include "stdf_define.h"
#include "stdf.h"

#include "stdf_app.h"
#include "stdf_os.h"

void stdf_init(void)
{
    STDF_LOG_I("start");

    stdf_os_init();
    stdf_app_init();
}
