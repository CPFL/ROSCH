/*
 * test_overhead.c:	test the scheduling overhead.
 */

#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/unistd.h>
#include <resch/api.h>
#include <resch-config.h>
#include "tvops.h"

struct task_data {
	struct timespec C;
	struct timespec T; 
	struct timespec D;
	int prio;
};

static struct timespec ms_to_timespec(unsigned long ms)
{
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms - ts.tv_sec*1000) * 1000000LL;
	return ts;
}

static void exec_task(int policy, struct task_data *p)
{
	int rid;
	long long l;

	/* initialization for using RESCH. */
	if ((rid = rt_init()) < 0) {
		printf("Error: cannot begin!\n");
		goto out;
	} 

	rt_set_runtime(p->C); 
	rt_set_wcet(p->C); 
	rt_set_period(p->T);
	rt_set_deadline(p->T);
	rt_set_scheduler(policy);
	rt_set_priority(p->prio);
	rt_run(p->T);
	for (l = 0; l < 10000000LL; l++) {
		asm volatile("nop");
	}
	rt_exit();
 out:
	_exit(1);
}

long test_context_switch(int policy)
{
	int i;
	cpu_set_t cpuset;
	struct timespec interval, wcet, runtime, period, timeout;
	struct timeval tv1, tv2, tv3;
	struct timeval tv_begin, tv_end;
	struct timeval tv_cost;
	long us_cost = 0;

	/* set a very short interval on purpose. */
	interval = ms_to_timespec(1);
	wcet = ms_to_timespec(500);
	runtime = wcet;
	period = ms_to_timespec(1000);
	timeout = ms_to_timespec(0);

	CPU_ZERO(&cpuset);
	CPU_SET(1, &cpuset);
	if (sched_setaffinity(0, sizeof(cpuset), &cpuset) == -1) {
		printf("Failed to migrate the process\n");
		return 0;
	}

	if (rt_init() < 0) {
		printf("Error: cannot begin!\n");
		return 0;
	}

	rt_set_runtime(wcet);
	rt_set_wcet(wcet);
	rt_set_period(period);
	rt_set_scheduler(policy);
	rt_set_priority(RESCH_APP_PRIO_MAX);
	rt_run(timeout);

	for (i = 0; i < 10; i++) {
		gettimeofday(&tv1, NULL);
		do  {
			gettimeofday(&tv2, NULL);
			/* tv2 - tv1 = tv3 */
			tvsub(&tv2, &tv1, &tv3);
		} while (tv3.tv_sec < 1);

		gettimeofday(&tv_begin, NULL);
		rt_wait_interval(interval);
		gettimeofday(&tv_end, NULL);

		tvsub(&tv_end, &tv_begin, &tv_cost);
		if (i > 0 && us_cost < tv_cost.tv_usec) {
			us_cost = tv_cost.tv_usec;
		}
	}

	rt_exit();

	return us_cost - 1000;
}

int test_job_release(int policy)
{
	int i, rid, status;
	int pid[NR_RT_TASKS];
	int cost = 0;
	unsigned long runtime = 50; /* ms */
	unsigned long period = runtime * (NR_RT_TASKS / NR_RT_CPUS); /* ms */
	struct task_data task;

	for (i = 1; i < NR_RT_TASKS; i++) {
		pid[i] = fork();
		if (pid[i] == 0) {
			task.C = ms_to_timespec(runtime);
			task.T = ms_to_timespec(period);
			task.D = task.T;
			if (RESCH_APP_PRIO_MAX - i > RESCH_APP_PRIO_MIN) {
				task.prio = RESCH_APP_PRIO_MAX - i;
			}
			else {
				task.prio = RESCH_APP_PRIO_MAX;
			}
			/* never returned. */
			exec_task(policy, &task);
		}
	}

	/* this should be after forking the tasks. */
	if ((rid = rt_init()) < 0) {
		printf("Error: cannot begin!\n");
		return 0;
	}

	rt_set_runtime(ms_to_timespec(runtime));
	rt_set_wcet(ms_to_timespec(runtime));
	rt_set_period(ms_to_timespec(period + 1));
	rt_set_scheduler(policy);
	rt_set_priority(RESCH_APP_PRIO_MAX);
	rt_run(ms_to_timespec(0));

	rt_wait_period();
	cost = rt_test_get_release_cost();

	for (i = 1; i < NR_RT_TASKS; i++) {
		pid[i] = wait(&status);
	}

	rt_exit();

	return cost;
}

long test_migration(int policy)
{
	int cost = 0;
	cpu_set_t cpuset;
	struct timespec interval, wcet, runtime, period, timeout;

	/* set a very short interval on purpose. */
	interval = ms_to_timespec(1);
	wcet = ms_to_timespec(500);
	runtime = wcet;
	period = ms_to_timespec(1000);
	timeout = ms_to_timespec(0);

	CPU_ZERO(&cpuset);
	CPU_SET(1, &cpuset);
	if (sched_setaffinity(0, sizeof(cpuset), &cpuset) == -1) {
		printf("Failed to migrate the process\n");
		return 0;
	}

	if (rt_init() < 0) {
		printf("Error: cannot begin!\n");
		return 0;
	}

	rt_set_runtime(wcet);
	rt_set_wcet(wcet);
	rt_set_period(period);
	rt_set_scheduler(policy);
	rt_set_priority(RESCH_APP_PRIO_MAX);
	rt_run(timeout);

	cost = rt_test_get_migration_cost();

	rt_exit();

	return cost;
}

int main(int argc, char* argv[])
{
	int i;
	long cost;
	int policy = SCHED_FP;

	for (i = 1; i < argc; i++) {
		if (strncmp(argv[i], "--edf", strlen("--edf")) == 0) {
			policy = SCHED_EDF;
		}
	}
	
	printf("Checking for the context switching cost... ");
	fflush(stdout);
	cost = test_context_switch(policy);	
	printf(" %lu microseconds\n", cost);
	rt_test_set_switch_cost(cost);

	printf("Checking for the job releasing cost... ");
	fflush(stdout);
	cost = test_job_release(policy);	
	printf(" %lu microseconds\n", cost);
	rt_test_set_release_cost(cost);

	printf("Checking for the migration cost... ");
	fflush(stdout);
	cost = test_migration(policy);	
	printf(" %lu microseconds\n", cost);
	rt_test_set_migration_cost(cost);

	printf("[Done]\n");

	return 0;
}
