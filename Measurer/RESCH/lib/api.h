#ifndef __LIB_API_H__
#define __LIB_API_H__
#include <sys/time.h>


/* supported scheduling policy. */
#define SCHED_FAIR	0
#define SCHED_FP	1
#define SCHED_EDF	2

/* available priority range. */
#define RESCH_APP_PRIO_MAX	96
#define RESCH_APP_PRIO_MIN	4

/* period value for non-perioid tasks. */
#define RESCH_PERIOD_INFINITY	0

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************
 * ROS APIs for real-time scheduling.
 ************************************************************/
int ros_rt_init(const char* node_name);
int ros_rt_set_node(unsigned long node_index);
int ros_rt_start_callback(unsigned long node_index);
int ros_rt_end_callback(unsigned long node_index);
int ros_rt_exit(void);
int ros_rt_run(struct timespec);
int ros_rt_wait_period(void);
int ros_rt_wait_interval(struct timespec);
int ros_rt_set_period(struct timespec);
int ros_rt_set_deadline(struct timespec);
int ros_rt_set_wcet(struct timespec);
int ros_rt_set_runtime(struct timespec);
int ros_rt_set_priority(unsigned long);
int ros_rt_set_scheduler(unsigned long);
int ros_rt_background(void);

/************************************************************
 * PORT-I APIs for preemptive periodic real-time scheduling.
 ************************************************************/
int rt_init(void);
int rt_exit(void);
int rt_run(struct timespec);
int rt_wait_period(void);
int rt_wait_interval(struct timespec);
int rt_set_period(struct timespec);
int rt_set_deadline(struct timespec);
int rt_set_wcet(struct timespec);
int rt_set_runtime(struct timespec);
int rt_set_priority(unsigned long);
int rt_set_scheduler(unsigned long);
int rt_background(void);

/*******************************************************
 * PORT-II APIs for event-driven asynchrous scheduling.
 *******************************************************/
int rt_sleep(struct timespec);
int rt_suspend(void);
int rt_wake_up(int);

/**************************************************
 * PORT-III APIs for reservation-based scheduling.
 **************************************************/
int rt_reserve_start(struct timespec, void (*)(void));
int rt_reserve_stop(void);
int rt_reserve_expire(void);
int rt_server_create(struct timespec, struct timespec);
int rt_server_run(void);

/*****************************************************
 *                 test functions                    *
 *****************************************************/
int rt_test_set_switch_cost(unsigned long);
int rt_test_set_release_cost(unsigned long);
int rt_test_set_migration_cost(unsigned long);
int rt_test_get_release_cost(void);
int rt_test_get_migration_cost(void);
int rt_test_get_runtime(void);
int rt_test_get_utime(void);
int rt_test_get_stime(void);
int rt_test_reset_stime(void);

#ifdef __cplusplus
}
#endif

#endif
