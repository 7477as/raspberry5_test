/* stdf_os - 顶层聚合 init（mem → msg → delay_msg），顺序不能换 */

#include "stdf_define.h"
#include "stdf_os.h"

void stdf_os_init(void)
{
    stdf_os_mem_init();
    stdf_os_msg_init();
    stdf_os_delay_msg_init();
}
