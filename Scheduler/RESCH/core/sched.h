#ifndef __RESCH_SCHED_H__
#define __RESCH_SCHED_H__

#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/semaphore.h>
#include "preempt_trace.h"
#include <resch-config.h>

/* return pointer to the resch task descriptor. */
#define resch_task_ptr(rid)	(&resch_task[rid])
/* return the reversed value of the given priority.
   this is used by priority arrays. */
#define prio_index(prio)	(RESCH_PRIO_MAX - prio)

/* priority array which includes only active (ready) tasks. */
#define RESCH_PRIO_LONG BITS_TO_LONGS(MAX_RT_PRIO)
struct prio_array {
	int nr_tasks;
	spinlock_t lock;
	unsigned long bitmap[RESCH_PRIO_LONG];
	struct list_head queue[RESCH_PRIO_MAX];
};

/* kernel thread structure that calls sched_setscheduler() exported
   from the Linux kernel to change the scheduling policy and the
   priority of the tasks.
   it is necessary because the Linux kernel does not allow user processes
   without root permissions to change the real-time priority.
   note that we may need to take into accout security matters... */
struct sched_thread_struct {
	struct task_struct *task;
	struct list_head list;
	spinlock_t lock;
};

/* bitmap for the process ID used in RESCH: RESCH ID (RID). */
#define PID_MAP_LONG BITS_TO_LONGS(NR_RT_TASKS)
struct pid_map_struct {
	unsigned long bitmap[PID_MAP_LONG];
	spinlock_t lock;
};

/* priority-ordered double-linked list */
struct task_list_struct {
	struct semaphore sem;
	struct list_head head;
};

/* local object for each CPU. */
struct local_object {
	struct prio_array active;
	struct sched_thread_struct sched_thread;
#ifdef RESCH_PREEMPT_TRACE
	struct resch_task_struct *current_task;
	spinlock_t preempt_lock;
	unsigned long last_tick;
#endif
};

/* externs. */
extern resch_task_t resch_task[NR_RT_TASKS];
extern struct local_object lo[NR_RT_CPUS + 1];

/* APIs. */
/* ROS APIs. */
int api_set_node(int, unsigned long);
int api_start_callback(int, unsigned long);
int api_end_callback(int, unsigned long);
/* General APIs. */
int api_init(void);
int api_exit(int);
int api_run(int, unsigned long);
int api_wait_for_period(int);
int api_wait_for_interval(int, unsigned long);
int api_set_period(int, unsigned long);
int api_set_deadline(int, unsigned long);
int api_set_wcet(int, unsigned long);
int api_set_runtime(int, unsigned long);
int api_set_priority(int, unsigned long);
int api_set_scheduler(int, unsigned long);
int api_background(int);

void sched_init(void);
void sched_exit(void);

#ifdef RESCH_HRTIMER
void hrtimer_sleep_interval(resch_task_t *, unsigned long);
#define sleep_interval(rt, interval) hrtimer_sleep_interval(rt, interval)
#else
#define sleep_interval(rt, interval) schedule_timeout(interval)
#endif

/* migrate the given task in Linux. */
static inline void __migrate_task(resch_task_t *rt, int cpu)
{
#ifdef USE_VIVID_OR_OLDER
	cpus_clear(rt->cpumask);
	cpu_set(cpu, rt->cpumask);
#else
cpumask_clear(&rt->cpumask);
cpumask_set_cpu(cpu,&rt->cpumask); 
#endif
	set_cpus_allowed_ptr(rt->task, &rt->cpumask);
}

/* round up function for unsigned long type. */
static inline unsigned long div_round_up(unsigned long x, unsigned long y)
{
	if (x % y == 0)
		return x / y;
	else
		return x / y + 1;
}

#endif
