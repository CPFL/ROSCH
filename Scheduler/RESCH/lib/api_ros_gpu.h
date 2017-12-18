#ifndef __LIB_ROS_GPU_H__
#define __LIB_ROS_GPU_H__

#include <sys/shm.h>
#include <pthread.h>
#if 0
#ifdef __cplusplus
extern "C"{
#endif
#endif
#include <queue>
#include <iostream>
#include <fstream>

int ros_gsched_init(void);
int ros_gsched_enqueue(void);
int ros_gsched_dequeue(void);
int ros_gsched_exit(bool);

extern pthread_mutex_t *launch_mutex;
extern pthread_mutex_t *value_mutex;
extern int *maxvalue;

extern int shm_launch_mutex_id;
extern int shm_value_mutex_id;
extern int shm_maxvalue_id;
#if 0
#ifdef __cplusplus
}
#endif
#endif
#endif
