/*
 * rt_task.c
 *
 * Sample program that executes one periodic real-time task.
 * Each job consumes 1 second, if it is not preempted.
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <resch/api.h>
#include "tvops.h"
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sched.h>
#include <linux/sched.h>

#include "config.h"

int cuda_test_madd(unsigned int n, char *path, int prio);
struct timespec period, runtime, timeout, deadline;
unsigned int prio;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static struct timespec ms_to_timespec(unsigned long ms)
{
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms - ts.tv_sec*1000) * 1000000LL;
	return ts;
}

void* thread(void *args){

#ifndef DISABLE_SCHED_CPU
  rt_init(); 
  rt_set_period(period);
  rt_set_deadline(period);// if period = deadline, don't call "rt_set_deadline"
	rt_set_runtime(runtime);
	rt_set_scheduler(SCHED_FP); /* you can also set SCHED_EDF. */ 
	rt_set_priority(prio);
	rt_run(timeout);
#endif

	cuda_test_madd(1000, ".", prio);
#ifndef DISABLE_SCHED_CPU
	rt_wait_period();
	rt_exit();
#endif
  return NULL;
}


int main(int argc, char* argv[])
{
	int i, j, task_num;
	struct timeval startTime, endTime;

	if (argc != 5) {
		printf("Error: invalid option\n");
		printf("usage: period(ms) runtime(ms) deadline(ms) task_num\n");
		exit(EXIT_FAILURE);
	}

#ifndef DISABLE_SCHED_CPU
	prio = 0;					/* priority. */
	//printf("---- period  :%d ms ----\n", atoi(argv[1]));
	period = ms_to_timespec(atoi(argv[1]));	/* period. */
	//printf("---- runtime :%d ms ----\n", atoi(argv[2]));
	runtime = ms_to_timespec(atoi(argv[2]));			/* execution time. */
	//printf("---- deadline:%d ms ----\n", atoi(argv[3]));
	deadline = ms_to_timespec( atoi(argv[3]) );			/* execution time. */
	//printf("---- timeout:%d ms ----\n", 30000);
	timeout = ms_to_timespec(3000);			/* timeout. */
#endif


	task_num = atoi(argv[4]); 

	int cpu;
	cpu_set_t set;
	cpu = sched_getcpu();
	CPU_ZERO(&set);
	CPU_SET(cpu,&set);

	gettimeofday(&startTime, NULL);
	pid_t pid;

	/* generate tasks */
	for(j=0; j<task_num; j++){
	    pid= fork();
	    //prio = DEFAULT_PRIO;
			prio = getpid();
	    if( -1 == pid)
	    {
				fprintf(stderr,"can not fork\n");
				break;
	    }
			else if( 0 == pid){
				fprintf(stderr,"fork! pid=%d\n",getpid());
				thread(NULL);
				exit(EXIT_SUCCESS);
	  	}
	}

	pthread_setaffinity_np(pthread_self() ,sizeof(cpu_set_t), &set);
	int status = 0;

	while(1){
	    pid = wait(&status);
	    if(pid == -1){
				if(ECHILD == errno){
		   		break;
				}
				else if (EINTR == errno)
				{
		    	fprintf(stderr,"synchronize\n");
		    	continue;
				}
	    err (EXIT_FAILURE, "wait error");
	    }

	    fprintf (stderr,"parenet: child = %d, status=%d\n", pid, status);
	}

	/* get execution time */
	gettimeofday(&endTime, NULL);      
	time_t diffsec = difftime(endTime.tv_sec, startTime.tv_sec);
	suseconds_t diffsub = endTime.tv_usec - startTime.tv_usec;
	double realsec = diffsec+diffsub*1e-6; 
	printf("Execution Time: %f \n",realsec);
	return 0;
}
