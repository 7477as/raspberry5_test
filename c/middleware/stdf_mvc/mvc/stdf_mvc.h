/* stdf_mvc middleware lifecycle. */
#ifndef __STDF_MVC_H__
#define __STDF_MVC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize middleware before application subscriptions and publications. */
int stdf_mvc_init(void);
/* Clear middleware state after application teardown and the final poll. */
void stdf_mvc_deinit(void);

#ifdef __cplusplus
}
#endif
#endif
