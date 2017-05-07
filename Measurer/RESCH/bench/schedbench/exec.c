/*
 * exec.c		Copyright (C) Shinpei Kato
 *
 * Execute periodic tasks corresponding to the given task set and 
 * measure the success ratio.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <resch/api.h>
#include "schedbench.h"

/* the number of column in taskset files. */
#define NR_COLS 	4 
/* the default timeout (us). */
#define DEFAULT_TIMEOUT	1000

static struct timespec ms_to_timespec(unsigned long ms)
{
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms - ts.tv_sec*1000) * 1000000LL;
	return ts;
}

typedef struct task_struct {
	pid_t pid;	/* linux process ID. */
	int tid;	/* task ID. */
	int C;		/* computation time */
	int T; 		/* period */
	int D;		/* relative deadline */
	int prio; 	/* priority */
} task_t;

/* task control blocks. */
task_t *tasks;
/* the number of tasks. */
int nr_tasks;

void exec_task(task_t *task, int loop_1ms, int time, int policy)
{
	int k; /* do not use i! */
	int ret = RET_SUCCESS;
	int nr_jobs = time / task->T;
	struct timespec C, T, D, timeout;

	C = ms_to_timespec(task->C);
	T = ms_to_timespec(task->T);
	D = ms_to_timespec(task->D);
	timeout = ms_to_timespec(DEFAULT_TIMEOUT);

	/* get own PID. */
	task->pid = getpid();

	/* initialization for using RESCH. */
	if (rt_init() < 0) {
		printf("Error: cannot begin!\n");
		ret = RET_MISS;
		goto out;
	} 

	rt_set_wcet(C); 
	rt_set_period(T);		
	rt_set_deadline(D);	
	/* rt_set_scheduler must be after setting timing properties. */
	rt_set_scheduler(policy);
	if (policy == SCHED_FP) {
		rt_set_priority(task->prio);
	}
	rt_run(timeout);

	/* busy loop. */
	for (k = 0; k < nr_jobs; k++) {
		LOOP(task->C * loop_1ms);
		if (!rt_wait_period()) {
			ret = RET_MISS;
			break;
		}
	}

	rt_exit();
 out:
	/* call _exit() but not exit(), since exit() may remove resources
	   that are shared with the parent and other child processes. */
	_exit(ret);

	/* no return. */
}

void init_task(task_t *t, int tid, ulong_t C, ulong_t T, ulong_t D)
{
	t->pid = 0;
	t->tid = tid;
	t->prio = 1; /* initial priority is 1. */
	t->C = C;
	t->T = T;
	t->D = D;
}

int read_taskset(FILE *fp)
{
	char line[MAX_BUF], s[MAX_BUF];
	char props[NR_COLS][MAX_BUF];
	ulong_t lcm;
	int tmp;
	int n; /* # of tasks */
	int i, j;
	char *token;
	ulong_t C, T, D;

	/* skip comments and empty lines */
	while(fgets(line, MAX_BUF, fp)) {
		if (line[0] != '\n'	&& line[0] != '#')
			break;
	}

	/* get the number of tasks */
	n = atoi(line);

	/* allocate the memory for the tasks */
	tasks = (task_t *) malloc(sizeof(task_t) * n);

	/* skip comments and empty lines */
	while(fgets(line, MAX_BUF, fp)) {
		if (line[0] != '\n'	&& line[0] != '#')
			break;
	}

	for (i = 0; i < n; i++) {
		strcpy(s, line);
		token = strtok(s, ",\t ");
		/* get task name, exec. time, period, and relative deadline. */
		for (j = 0; j < NR_COLS; j++) {
			if (!token) {
				printf("Error: invalid format: %s!\n", line);
				exit(1);
			}
			strncpy(props[j], token, MAX_BUF);
			token = strtok(NULL, ",\t ");
		}
		C = atoi(props[1]);
		T = atoi(props[2]);
		D = atoi(props[3]);
		init_task(&tasks[i], i, C, T, D);
		// printf("task%d: %d, %d, %d\n", i, C, T, D);

		/* get line for the next task.  */
		fgets(line, MAX_BUF, fp);
	}

	return n;
}

void deadline_monotonic_priority(void)
{
	int i, j;
	ulong_t prio;
	task_t task;

	/* bable sort such that tasks[j-1].D <= tasks[j].D. */
	for (i = 0; i < nr_tasks - 1; i++) {
		for (j = nr_tasks - 1; j > i; j--) {
			if (tasks[j-1].D > tasks[j].D) {
				task.tid 	= tasks[j].tid;
				task.C 		= tasks[j].C;
				task.T 		= tasks[j].T;
				task.D 		= tasks[j].D;

				tasks[j].tid 	= tasks[j-1].tid;
				tasks[j].C 		= tasks[j-1].C;
				tasks[j].T 		= tasks[j-1].T;
				tasks[j].D 		= tasks[j-1].D;

				tasks[j-1].tid 	= task.tid;
				tasks[j-1].C 	= task.C;
				tasks[j-1].T 	= task.T;
				tasks[j-1].D 	= task.D;
			}
		}
	}

	/* set priorities. */
	prio = RESCH_APP_PRIO_MAX;
	for (i = 0; i < nr_tasks; i++) {
		tasks[i].prio = prio;
		prio--;
	}
}

int schedule(FILE *fp, int m, int loop_1ms, int time, int policy)
{
	int i, j, ret, status;
	pid_t pid;

	/* task[] must be freed in the end. */
	nr_tasks = read_taskset(fp);
	deadline_monotonic_priority();

	/* all the global variables must be set before.
	   otherwise, the child processes cannot share the variables. */
	for (i = 0; i < nr_tasks; i++) {
		pid = fork();
		if (pid == 0) { /* the child process. */
			/* execute the task. 
			   note that the funtion never returns. */
			exec_task(&tasks[i], loop_1ms, time, policy);
		}
	}

	/* default return value. */
	ret = TRUE;

	/* wait for the child processes. */
	for (i = 0; i < nr_tasks; i++) {
		pid = wait(&status);
		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status) == RET_MISS) {
				ret = FALSE;
			}
		}
		else {
			printf("Anomaly exit.\n");
			ret = FALSE;
		}
	}

 out:
	free(tasks);

	return ret;
}
