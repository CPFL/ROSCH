/*
 *  libresch.c: the library for the RESCH core
 *
 * 	Defines application programming interfaces (APIs) for RESCH.
 */

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/unistd.h>
#include <resch-api.h>
#include "api.h"

#define discard_arg(arg)	asm("" : : "r"(arg))
#define UNDEFINED_RID -1

int rid = UNDEFINED_RID;
void (*user_xcpu_handler)(void) = NULL;

static void xcpu_handler(int signum)
{
	discard_arg(signum);
	rt_reserve_expire();
	if (user_xcpu_handler) {
		user_xcpu_handler();
	}
}

static void kill_handler(int signum)
{
	discard_arg(signum);
	rt_exit();
	/* use kill() instead of exit(). */
	kill(0, SIGINT);
}

/**
 * internal function for APIs with no arguments.
 * it uses write() system call to pass the information to the kernel.
 */
static inline int __api(int api)
{
	int fd, ret;
	struct api_user_struct a;

	fd = open(RESCH_DEVNAME, O_RDWR);
	if (fd < 0) {
		printf("Error: failed to access the module!\n");
		return RES_FAULT;
	}
	a.api = api;
	a.rid = rid;
	ret = write(fd, &a, sizeof(a));
	close(fd);

	return ret;
}

/**
 * internal function for APIs with integer arguments.
 * it uses write() system call to pass the information to the kernel.
 */
static inline int __api_int(int api, int val)
{
	int fd, ret;
	struct api_int_user_struct a;

	fd = open(RESCH_DEVNAME, O_RDWR);
	if (fd < 0) {
		printf("Error: failed to access the module!\n");
		return RES_FAULT;
	}
	a.api = api;
	a.rid = rid;
	a.val = val;
	ret = write(fd, &a, sizeof(a));
	close(fd);

	return ret;
}

/**
 * internal function for APIs with timespec arguments.
 * it uses write() system call to pass the information to the kernel.
 */
static inline int __api_ts(int api, struct timespec ts)
{
	int fd, ret;
	struct api_ts_user_struct a;

	fd = open(RESCH_DEVNAME, O_RDWR);
	if (fd < 0) {
		printf("Error: failed to access the module!\n");
		return RES_FAULT;
	}
	a.api = api;
	a.rid = rid;
	a.ts = ts;
	ret = write(fd, &a, sizeof(a));
	close(fd);

	return ret;
}

/**
 * internal function for tests, using ioctl() system call.
 */
static inline int __test(unsigned long cmd, unsigned long val)
{
	int fd, ret;

	fd = open(RESCH_DEVNAME, O_RDWR);
	if (fd < 0) {
		printf("Error: failed to access the module!\n");
		return RES_FAULT;
	}
	ret = ioctl(fd, cmd, &val);
	close(fd);

	return ret;
}

/************************************************************
 * ROS APIs for real-time scheduling.
 ************************************************************/

int ros_rt_init(const char* node_name)
{
#if 0
    /* Failed */
    if ((__api_int(API_SET_NODE, node_index) == RES_FAULT) ? 0 : 1) {
        return 0;
    }
    /* Success */
    else {
       return rt_init();
    }
#endif
    return rt_init();
}
int ros_rt_exit(void)
{
    return rt_exit();
}
int ros_rt_run(struct timespec ts)
{
    return rt_run(ts);
}
int ros_rt_wait_period(void)
{
    return rt_wait_period();
}
int ros_rt_wait_interval(struct timespec ts)
{
    return rt_wait_interval(ts);
}
int ros_rt_set_period(struct timespec ts)
{
    return rt_set_period(ts);
}
int ros_rt_set_deadline(struct timespec ts)
{
    return rt_set_deadline(ts);
}
int ros_rt_set_wcet(struct timespec ts)
{
    return rt_set_wcet(ts);
}
int ros_rt_set_runtime(struct timespec ts)
{
    return rt_set_runtime(ts);
}
int ros_rt_set_priority(unsigned long prio)
{
    return rt_set_priority(prio);
}
int ros_rt_set_scheduler(unsigned long sched_policy)
{
    return rt_set_scheduler(sched_policy);
}
int ros_rt_background(void)
{
    return rt_background();
}
int ros_rt_set_node(unsigned long node_index)
{
    return (__api_int(API_SET_NODE, node_index) == RES_FAULT) ? 0 : 1;
}
int ros_rt_start_callback(unsigned long node_index)
{
    return (__api_int(API_START_CALLBACK, node_index) == RES_FAULT) ? 0 : 1;
}
int ros_rt_end_callback(unsigned long node_index)
{
    return (__api_int(API_END_CALLBACK, node_index) == RES_FAULT) ? 0 : 1;
}

/************************************************************
 * PORT-I APIs for preemptive periodic real-time scheduling.
 ************************************************************/

int rt_init(void)
{
	struct sigaction sa_kill;
	struct sigaction sa_xcpu;

	/* register the KILL signal. */
	memset(&sa_kill, 0, sizeof(sa_kill));
	sigemptyset(&sa_kill.sa_mask);
	sa_kill.sa_handler = kill_handler;
	sa_kill.sa_flags = 0;
	sigaction(SIGINT, &sa_kill, NULL);

	/* register the XCPU signal. */
	memset(&sa_xcpu, 0, sizeof(sa_xcpu));
	sigemptyset(&sa_xcpu.sa_mask);
	sa_xcpu.sa_handler = xcpu_handler;
	sa_xcpu.sa_flags = 0;
	sigaction(SIGXCPU, &sa_xcpu, NULL);

	rid = __api(API_INIT);
	return rid;
}

int rt_exit(void)
{
	struct sigaction sa_kill;
	struct sigaction sa_xcpu;

	/* unregister the KILL signal. */
	memset(&sa_kill, 0, sizeof(sa_kill));
	sigemptyset(&sa_kill.sa_mask);
	sa_kill.sa_handler = SIG_DFL;
	sa_kill.sa_flags = 0;
	sigaction(SIGINT, &sa_kill, NULL);

	/* unregister the XCPU signal. */
	memset(&sa_xcpu, 0, sizeof(sa_xcpu));
	sigemptyset(&sa_xcpu.sa_mask);
	sa_xcpu.sa_handler = SIG_DFL;
	sa_xcpu.sa_flags = 0;
	sigaction(SIGXCPU, &sa_xcpu, NULL);

	return (__api(API_EXIT) == RES_FAULT) ? 0 : 1;
}

int rt_run(struct timespec ts)
{
	return (__api_ts(API_RUN, ts) == RES_FAULT) ? 0 : 1;
}

int rt_wait_period(void)
{
	return (__api(API_WAIT_PERIOD) == RES_MISS) ? 0 : 1;
}

int rt_wait_interval(struct timespec ts)
{
	return (__api_ts(API_WAIT_INTERVAL, ts) == RES_MISS) ? 0 : 1;
}

int rt_set_period(struct timespec ts)
{
	return (__api_ts(API_SET_PERIOD, ts) == RES_FAULT) ? 0 : 1;
}

int rt_set_deadline(struct timespec ts)
{
	return (__api_ts(API_SET_DEADLINE, ts) == RES_FAULT) ? 0 : 1;
}

int rt_set_wcet(struct timespec ts)
{
	return (__api_ts(API_SET_WCET, ts) == RES_FAULT) ? 0 : 1;
}

int rt_set_runtime(struct timespec ts)
{
	return (__api_ts(API_SET_RUNTIME, ts) == RES_FAULT) ? 0 : 1;
}

int rt_set_priority(unsigned long prio)
{
	return (__api_int(API_SET_PRIORITY, prio) == RES_FAULT) ? 0 : 1;
}

int rt_set_scheduler(unsigned long policy)
{
	return (__api_int(API_SET_SCHEDULER, policy) == RES_FAULT) ? 0 : 1;
}

int rt_background(void)
{
	return (__api(API_BACKGROUND) == RES_FAULT) ? 0 : 1;
}

/*******************************************************
 * PORT-II APIs for event-driven asynchrous scheduling.
 *******************************************************/

int rt_sleep(struct timespec ts)
{
	return (__api_ts(API_SLEEP, ts) == RES_FAULT) ? 0 : 1;
}

int rt_suspend(void)
{
	return (__api(API_SUSPEND) == RES_FAULT) ? 0 : 1;
}

int rt_wake_up(int pid)
{
	return (__api_int(API_WAKE_UP, pid) == RES_FAULT) ? 0 : 1;
}

/**************************************************
 * PORT-III APIs for reservation-based scheduling.
 **************************************************/

int rt_reserve_start(struct timespec ts, void (*func)(void))
{
	if (func) {
		/* set a user handler. */
		user_xcpu_handler = func;
		return (__api_ts(API_RESERVE_START_XCPU, ts) == RES_FAULT) ? 0 : 1;
	}
	else {
		return (__api_ts(API_RESERVE_START, ts) == RES_FAULT) ? 0 : 1;
	}
}

int rt_reserve_stop(void)
{
	return (__api(API_RESERVE_STOP) == RES_FAULT) ? 0 : 1;
}

int rt_reserve_expire(void)
{
	return (__api(API_RESERVE_EXPIRE) == RES_FAULT) ? 0 : 1;
}

int rt_server_create(struct timespec period, struct timespec budget)
{
	if (!rt_set_period(period))
		return 0;
	if (!rt_reserve_start(budget, NULL))
		return 0;
	return 1;
}

int rt_server_run(void)
{
	return (__api(API_SERVER_RUN) == RES_FAULT) ? 0 : 1;
}

/*****************************************************
 *                 test functions                    *
 *****************************************************/

int rt_test_set_switch_cost(unsigned long us)
{
	return __test(TEST_SET_SWITCH_COST, us);
}

int rt_test_set_release_cost(unsigned long us)
{
	return __test(TEST_SET_RELEASE_COST, us);
}

int rt_test_set_migration_cost(unsigned long us)
{
	return __test(TEST_SET_MIGRATION_COST, us);
}

int rt_test_get_release_cost(void)
{
	return __test(TEST_GET_RELEASE_COST, rid);
}

int rt_test_get_migration_cost(void)
{
	return __test(TEST_GET_MIGRATION_COST, rid);
}

int rt_test_get_runtime(void)
{
	return __test(TEST_GET_RUNTIME, 0);
}

int rt_test_get_utime(void)
{
	return __test(TEST_GET_UTIME, 0);
}

int rt_test_get_stime(void)
{
	return __test(TEST_GET_STIME, 0);
}

int rt_test_reset_stime(void)
{
	return __test(TEST_RESET_STIME, 0);
}
