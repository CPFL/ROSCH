#ifndef __RESCH_GPU_LOCK_H__
#define __RESCH_GPU_LOCK_H__

#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/types.h>

struct gdev_lock {
    spinlock_t lock;
};

struct gdev_mutex {
    struct mutex mutex;
};

typedef struct gdev_lock gdev_lock_t;
typedef struct gdev_mutex gdev_mutex_t;


#endif
