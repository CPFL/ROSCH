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
#define gettime(fmt) clock_gettime(CLOCK_MONOTONIC, fmt)

static struct timespec ns_to_timespec(unsigned long ns)
{
	struct timespec ts;
	ts.tv_sec = ns / 1000000000;
	ts.tv_nsec = (ns - ts.tv_sec * 1000000000LL)  ;
	return ts;
}

static unsigned long long timespec_to_ns_sub(struct timespec *ts1, struct timespec *ts2)
{
	unsigned long long ret, ret2;
	ret = ts1->tv_sec * 10^9;
	ret2 = ts2->tv_sec * 10^9;
	ret += ts1->tv_nsec;
	ret2 += ts2->tv_nsec;

	ret = ret2 -ret;	
	return ret;
}


static struct timespec ms_to_timespec(unsigned long ms)
{
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms - ts.tv_sec * 1000) * 1000000LL;
	return ts;
}

int cuda_test_madd(unsigned int n, char *path, int prio);

struct timespec period, runtime, timeout, deadline;

	unsigned long prio;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void* thread(void *args)
{
    struct timespec ts1,ts2,ts3;
    int i;  


    gettime(&ts1);

#ifdef USE_SCHED_CPU
    rt_init(); 
    rt_set_period(period);
    rt_set_deadline(period);// if period = deadline, don't call "rt_set_deadline"
    rt_set_runtime(runtime);

#endif

    cuda_test_madd(1000, ".", prio);
    
    gettime(&ts2);

   // pthread_mutex_lock(&mutex);
   // printf("%ld,",timespec_to_ns_sub(&ts1,&ts2));
   // pthread_mutex_unlock(&mutex);
    return NULL;
}


int main(int argc, char* argv[])
{
	int i, j, task_num;
	struct timeval tv1, tv2, tv3;
	int dmiss_count = 0;
	pthread_t th[100];

	if (argc != 5) {
		printf("Error: invalid option\n");
		printf("usage: rt_task period runtime deadline task_num\n");
		exit(EXIT_FAILURE);
	}

#ifdef USE_SCHED_CPU
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
	prio = getpid();
	struct timespec ts1,ts2,ts3;

	task_num = atoi(argv[4]); 

	int cpu;
	cpu_set_t set;
	cpu = sched_getcpu();
	CPU_ZERO(&set);
	CPU_SET(cpu,&set);

	gettime(&ts1);
#if 0
	for(j=0; j<task_num; j++)
	    pthread_create(&th[j], NULL, thread, (void *)NULL);
	
	pthread_setaffinity_np(pthread_self() ,sizeof(cpu_set_t), &set);
	
	for(j=0; j<task_num; j++)
	    pthread_join(th[j],NULL);
#else /* generate each job by using fork    */
	pid_t pid;
	
	for(j=0; j<task_num; j++){
	    pid= fork();
	    prio = DEFAULT_PRIO;
	    if( -1 == pid)
	    {
		fprintf(stderr,"can not fork\n");
		break;
	    }else if( 0 == pid){
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

#endif

	gettime(&ts2);
	fprintf(stderr,"total_time = %llu\n",timespec_to_ns_sub(&ts1,&ts2));
	fprintf(stdout,"%llu\n",timespec_to_ns_sub(&ts1,&ts2));

	return 0;
}
