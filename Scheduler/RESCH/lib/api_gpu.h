#ifndef __LIB_API_GPU_H__
#define __LIB_API_GPU_H__

#include <stdint.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>


#define GDEV_IOCTL_CTX_CREATE 201
#define GDEV_IOCTL_LAUNCH 202
#define GDEV_IOCTL_SYNC 203
#define GDEV_IOCTL_CLOSE 204
#define GDEV_IOCTL_NOTIFY 205
#define GDEV_IOCTL_SETCID 206
#define GDEV_IOCTL_GETDEV 207

#define GPU_SCHED_FLAG_INIT   0x01
#define GPU_SCHED_FLAG_OPEN   0x02
#define GPU_SCHED_FLAG_LAUNCH 0x04
#define GPU_SCHED_FLAG_SYNCH  0x08
#define GPU_SCHED_FLAG_CLOSE  0x10

#define GSYNC_FENCE_SPIN  0x01
#define GSYNC_FENCE_YIELD 0x02
#define GSYNC_NOTIFY 	 0x10

#define DEFAULT_GSYNC_FLAG GSYNC_NOTIFY

#ifdef __cplusplus
extern "C" {
#endif

struct rtxGhandle{
    uint32_t dev_id;
    uint32_t vdev_id;
    uint32_t cid;
    void *task;
    void *nvdesc;
    uint8_t sched_flag;
    uint32_t setcid;
    uint8_t sync_flag;
};

/************************************************************
 * APIs for GPU scheduling.
 ************************************************************/

int rtx_gpu_init(void);
int rtx_gpu_exit(void);
int rtx_gpu_open(struct rtxGhandle **arg, unsigned int dev_id, unsigned int vdev_id);
int rtx_gpu_launch(struct rtxGhandle **arg);
int rtx_gpu_sync(struct rtxGhandle **arg);
int rtx_gpu_close(struct rtxGhandle **arg);

int rtx_gpu_notify(struct rtxGhandle **arg);
int rtx_gpu_setcid(struct rtxGhandle **arg, int new_cid);
int rtx_gpu_device_advice(struct rtxGhandle **arg);

/************************************************************
 * APIs for NVRM Interrupt
 ************************************************************/

int rtx_nvrm_init(struct rtxGhandle **arg, int dev_id);
int rtx_nvrm_notify(struct rtxGhandle **arg);
int rtx_nvrm_fence_poll(struct rtxGhandle **arg, uint8_t flag, uint32_t sequence);
int rtx_nvrm_fence(struct rtxGhandle **arg);
int rtx_nvrm_set_syncflag(struct rtxGhandle **arg);
int rtx_nvrm_close(struct rtxGhandle **arg);
#ifdef __cplusplus
}
#endif

#endif
