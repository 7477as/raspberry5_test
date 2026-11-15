/* stdf_mvc core lifecycle. */
#ifndef __STDF_MVC_CORE_H__
#define __STDF_MVC_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Install platform operations and reset values, pending events and subscriptions. */
int stdf_mvc_core_init(void);
/* Reset core state after publishers and polling have stopped. */
void stdf_mvc_core_deinit(void);

#ifdef __cplusplus
}
#endif
#endif
