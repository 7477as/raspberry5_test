/* stdf_app_net application initialization. */
#ifndef __STDF_APP_NET_H__
#define __STDF_APP_NET_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Start network sampling; return zero on success. */
int stdf_app_net_init(void);
/* Stop and join network sampling. */
void stdf_app_net_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_APP_NET_H__ */
