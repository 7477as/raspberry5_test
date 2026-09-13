/* stdf_os_mem - OS 内存池初始化（占位，Linux 走标准 malloc 暂不抽） */

#ifndef __STDF_OS_MEM_H__
#define __STDF_OS_MEM_H__

#ifdef __cplusplus
extern "C" {
#endif

void stdf_os_mem_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDF_OS_MEM_H__ */
