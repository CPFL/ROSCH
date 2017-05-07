#ifndef __RTX_H__
#define __RTX_H__

/* currently two scheduling policies are supported. */
#define RTX_SCHED_FP	0
#define	RTX_SCHED_EDF	1

/******************************* AIRS *******************************/
#if defined(AIRS)
#include <resch/api.h>
#include <sched.h>
static inline void rtx_init(void)
{
	rt_init();
}

static inline void rtx_exit(void)
{
	rt_exit();
}

static inline void rtx_wait_interval(struct timespec interval)
{
	rt_wait_interval(interval);
}

static inline void rtx_set_scheduler(struct timespec deadline, 
									 struct timespec period, 
									 struct timespec wcet,
									 struct timespec runtime,
									 int policy)
{
	rt_set_deadline(deadline);
	rt_set_period(period);
	rt_set_wcet(wcet);
	rt_set_runtime(runtime);
	switch (policy) {
	case RTX_SCHED_FP:
		rt_set_scheduler(SCHED_FP);
		break;
	case RTX_SCHED_EDF:
		rt_set_scheduler(SCHED_EDF);
		break;
	default:
		printf("Error: unknown scheduling policy.\n");
	}
}

static inline void rtx_reserve_start(struct timespec runtime)
{
	rt_reserve_start(runtime, NULL);
}

static inline void rtx_reserve_stop(void)
{
	rt_reserve_stop();
}

static inline void rtx_run(struct timespec timeout)
{
	rt_run(timeout);
}

/************************** Linux-Deadline ***************************/
#elif defined(DEADLINE)
#include <unistd.h>
#define __NR_sched_setscheduler_ex      299
#define __NR_sched_setparam_ex          300
#define __NR_sched_getparam_ex          301
#define __NR_sched_wait_interval        302
#define SCHED_DEADLINE 6
struct sched_param_ex {
	int sched_priority;
	struct timespec sched_runtime;
	struct timespec sched_deadline;
	struct timespec sched_period;
	int sched_flags;
};

long sched_setscheduler_ex(int pid, int policy, unsigned len,
						   struct sched_param_ex *param)
{
	return syscall(__NR_sched_setscheduler_ex, pid, policy, len, param);
}

long sched_wait_interval(int flags, 
						 const struct timespec *rqtp,
						 struct timespec *rmtp)
{
	return syscall(__NR_sched_wait_interval, flags, rqtp, rmtp);
}

static inline void rtx_init(void)
{
}

static inline void rtx_exit(void)
{
}

static inline void rtx_wait_interval(struct timespec interval)
{
	sched_wait_interval(!TIMER_ABSTIME, &interval, NULL);
}

/* By default, we cannot change the execution mode of a process without
   root permissions. That is, you should run real-time applications with 
   root permissions, which may not be preferred in many cases. So we can
   instead use the AIRS APIs, if installed, that enable us to change the 
   execution mode of a process without root permissions. If you want to 
   do this, switch #if 1 to #if 0 below. */
#if 1
static inline void rtx_set_scheduler(struct timespec deadline, 
									 struct timespec period, 
									 struct timespec wcet,
									 struct timespec runtime,
									 int policy)
{
	struct sched_param_ex spx;
	spx.sched_priority = 0;
	spx.sched_deadline = deadline;
	spx.sched_period = period;
	spx.sched_runtime = runtime;
	spx.sched_flags = 0;
	if (sched_setscheduler_ex(0, SCHED_DEADLINE, sizeof(spx), &spx) < 0) {
		printf("sched_setscheduler_ex() failed!\n");
	};
}
#else
static inline void rtx_set_scheduler(struct timespec deadline, 
									 struct timespec period, 
									 struct timespec wcet,
									 struct timespec runtime,
									 int policy)
{
	rt_set_deadline(deadline);
	rt_set_period(period);
	rt_set_wcet(wcet);
	rt_set_runtime(runtime);
	rt_set_scheduler(SCHED_EDF);
}
#endif

static inline void rtx_reserve_start(struct timeval *tv_runtime)
{
}

static inline void rtx_reserve_stop(void)
{
}

static inline void rtx_run(struct timespec timeout)
{
	/* wait for timeout. */
	usleep(timeout.tv_sec * 1000000 + timeout.tv_nsec / 1000);
}

/***************************** Linux/RK ******************************/
#elif defined(RK)
#define __USE_GNU
#include <sched.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <rk/rk.h> /* you should have the path to the header file. */

/* We need to fix this: #define __RK__  */
#define __NR_rk_resource_set_create                             333
#define __NR_rk_resource_set_destroy                            334
#define __NR_rk_cpu_reserve_create                              335
#define __NR_rt_wait_for_next_period                            336
#define __NR_rk_setschedulingpolicy                             337
#define __NR_rk_resource_set_attach_process                     338
#define __NR_rk_get_start_of_current_period                     339
#define __NR_rk_get_current_time                                340
#define __NR_rk_is_scheduling                                   341
#define __NR_rk_distributed_resource_container_create           342
#define __NR_rk_distributed_resource_set_create                 343
#define __NR_rk_distributed_resource_set_destroy                344
#define __NR_rk_distributed_cpu_reserve_create                  345
#define __NR_rk_resource_set_detach_process                     346
#define __NR_rk_pi_mutex_create                                 347
#define __NR_rk_pi_mutex_lock                                   348
#define __NR_rk_pi_mutex_unlock                                 349
#define __NR_rk_pi_mutex_destroy                                350
#define __NR_rk_pcp_mutex_create                                351
#define __NR_rk_pcp_mutex_lock                                  352
#define __NR_rk_pcp_mutex_unlock                                353
#define __NR_rk_pcp_mutex_destroy                               354

#define rk_cpu_reserve_create(x, y) \
	syscall(__NR_rk_cpu_reserve_create, x, y)
#define rk_distributed_resource_container_create(x) \
	syscall(__NR_rk_distributed_resource_container_create, x)
#define rk_get_current_time(x) \
	syscall(__NR_rk_get_current_time, x)
#define rk_get_start_of_current_period(x) \
	syscall(__NR_rk_get_start_of_current_period, x)
#define rk_is_scheduling() \
	syscall(__NR_rk_is_scheduling)
#define rk_resource_set_attach_process(x, y) \
	syscall(__NR_rk_resource_set_attach_process, x, y)
#define rk_resource_set_create(x) \
	syscall(__NR_rk_resource_set_create, x)
#define rk_resource_set_destroy(x) \
	syscall(__NR_rk_resource_set_destroy, x)
#define rk_setschedulingpolicy() \
	syscall(__NR_rk_setschedulingpolicy)
#define rt_wait_for_next_period() \
	syscall(__NR_rt_wait_for_next_period)
#define rk_pi_mutex_create() \
	syscall(__NR_rk_pi_mutex_create)
#define rk_pi_mutex_lock() \
	syscall(__NR_rk_pi_mutex_lock)
#define rk_pi_mutex_unlock() \
	syscall(__NR_rk_pi_mutex_unlock)
#define rk_pi_mutex_destroy() \
	syscall(__NR_rk_pi_mutex_destroy)
#define rk_pcp_mutex_create() \
	syscall(__NR_rk_pcp_mutex_create)
#define rk_pcp_mutex_lock() \
	syscall(__NR_rk_pcp_mutex_lock)
#define rk_pcp_mutex_unlock() \
	syscall(__NR_rk_pcp_mutex_unlock)
#define rk_pcp_mutex_destroy() \
	syscall(__NR_rk_pcp_mutex_destroy)

static rk_resource_set_t rs = NULL;
static inline void rtx_init(void)
{
}

static inline void rtx_exit(void)
{
	if(rs != NULL) {
		rk_resource_set_destroy(rs);
	}
}

static inline void rtx_wait_interval(struct timespec interval)
{
	rt_wait_for_next_period();
}

static inline void rtx_set_scheduler(struct timespec deadline, 
									 struct timespec period, 
									 struct timespec wcet,
									 struct timespec runtime,
									 int policy)
{
	struct cpu_reserve_attr cpu_attr;
	struct timespec ts_zero = {0, 0};
	struct timeval tv;
	char s[32];

	cpu_attr.deadline = deadline;
	cpu_attr.period = period;
	cpu_attr.compute_time = runtime;
	cpu_attr.blocking_time = ts_zero;
	cpu_attr.start_time = ts_zero;

	cpu_attr.reserve_type.sch_mode = RSV_HARD;
	cpu_attr.reserve_type.enf_mode = RSV_SOFT;
	cpu_attr.reserve_type.rep_mode = RSV_SOFT;
	
	gettimeofday(&tv, NULL);
	sprintf(s, "%12l:%12l", tv.tv_sec, tv.tv_usec);
	rs = rk_resource_set_create(s);
	rk_cpu_reserve_create(rs, &cpu_attr);
}

static inline void rtx_reserve_start(struct timespec runtime)
{
}

static inline void rtx_reserve_stop(void)
{
}

static inline void rtx_run(struct timespec timeout)
{
	/* wait for timeout. */
	usleep(timeout.tv_sec * 1000000 + timeout.tv_nsec / 1000);
	rk_resource_set_attach_process(rs, getpid());	
	rt_wait_for_next_period();	
}

/**************************** LITMUS^RT ****************************/
#elif defined(LITMUS) /* you should link the liblitmus. */
#include <litmus/litmus.h> /* you should have the path to the header file. */

/* execution cost and period. */
lt_t e, p;

static inline void rtx_init(void)
{
	init_litmus();
}

static inline void rtx_exit(void)
{
	task_mode(BACKGROUND_TASK);
}

static inline void rtx_wait_interval(struct timespec interval)
{
	sleep_next_period();
}

static inline void rtx_set_scheduler(struct timespec deadline, 
									 struct timespec period, 
									 struct timespec wcet,
									 struct timespec runtime,
									 int policy)
{
	p = period.tv_sec * 1000000000LL + period.tv_nsec;
	e = runtime.tv_sec * 1000000000LL + runtime.tv_nsec;
}

static inline void rtx_reserve_start(struct timespec runtime)
{
}

static inline void rtx_reserve_stop(void)
{
}

static inline void rtx_run(struct timespec timeout)
{
	/* wait for timeout. */
	usleep(timeout.tv_sec * 1000000 + timeout.tv_nsec / 1000);
	sporadic_global_ns(e, p);
	task_mode(LITMUS_RT_TASK);
}

/****************************** VANILLA ******************************/
#else

static inline void rtx_init(void)
{
}

static inline void rtx_exit(void)
{
}

static inline void rtx_wait_interval(struct timespec interval)
{
}

static inline void rtx_set_scheduler(struct timespec deadline, 
									 struct timespec period, 
									 struct timespec wcet,
									 struct timespec runtime,
									 int policy)
{
}

static inline void rtx_reserve_start(struct timespec runtime)
{
}

static inline void rtx_reserve_stop(void)
{
}

static inline void rtx_run(struct timespec timeout)
{
	/* wait for timeout. */
	usleep(timeout.tv_sec * 1000000 + timeout.tv_nsec / 1000);
}

#endif

#endif
