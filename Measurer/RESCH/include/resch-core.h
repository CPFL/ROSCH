#ifndef __RESCH_CORE_H__
#define __RESCH_CORE_H__
#include <linux/sched.h>
#include <linux/time.h>
#include <resch-config.h>
#include <linux/version.h>

/* priority levels. 
   note that migration_thread in the kernel should have the top level. */
#define RESCH_PRIO_MAX			(MAX_RT_PRIO - 2) 
#define RESCH_PRIO_MIN			(1)
#define RESCH_PRIO_BACKGROUND	(RESCH_PRIO_MIN + 1)
#define RESCH_PRIO_KTHREAD 		(RESCH_PRIO_MAX)
#define RESCH_PRIO_EDF 			(RESCH_PRIO_BACKGROUND + 1)
#define RESCH_PRIO_EDF_RUN 		(RESCH_PRIO_EDF + 1)

/* supported scheduling policies. */
#define RESCH_SCHED_FAIR	0
#define RESCH_SCHED_FP		1
#define RESCH_SCHED_EDF		2

/* used if tasks are not assigned a particular CPU. */
#define RESCH_CPU_UNDEFINED NR_RT_CPUS

/* verify if the task is managed (in the global list). */
#define task_is_managed(rt) (!list_empty(&(rt)->global_entry))
/* verify if the task is in the active queue. */
#define task_is_active(rt) (!list_empty(&(rt)->active_entry))
/* verify if the task has been submitted in RESCH. */
#define task_is_submitted(rt) ((rt)->release_time > 0)
/* verify if a job of the task is started but not completed. */
#define job_is_started(rt) ((rt)->exec_time > 0)
/* true iff @rt has a reserve. */
#define task_has_reserve(rt) ((rt)->reserve_time > 0)
/* verify if the task is accounting on the reserve. */
#define task_is_accounting(rt) (rt)->accounting
/* true iff @rt is assigned to the given CPU. */
#define task_is_on_cpu(rt, cpu)	((rt)->cpu_id == cpu)

#define DEBUG_PRINT
/* for debug */
#ifdef DEBUG_PRINT
#define RESCH_DPRINT(fmt,arg...) printk(KERN_INFO "[RESCH]:" fmt, ##arg)
#else
#define RESCH_DPRINT(fmt,arg...)
#endif

typedef struct resch_task_struct resch_task_t;
typedef struct server_struct server_t;

/**
 * server control block.
 */
struct server_struct {
	unsigned long capacity;
	resch_task_t *rt;
	struct timer_list timer;
};

/**
 * setscheduler struct. 
 */
struct setscheduler_struct {
	int prio;
	struct task_struct *caller;
};

/**
 * internal scheduling class.
 */
struct resch_sched_class {
	int (*set_scheduler) (resch_task_t *, int);
	void (*enqueue_task) (resch_task_t *, int, int);
	void (*dequeue_task) (resch_task_t *, int, int);
	void (*job_start) (resch_task_t *);
	void (*job_complete) (resch_task_t *);
	void (*migrate_task) (resch_task_t *, int);
	void (*reserve_replenish) (resch_task_t *, unsigned long);
	void (*reserve_expire) (resch_task_t *);
	void (*start_account) (resch_task_t *);
	void (*stop_account) (resch_task_t *);
	void (*wait_period) (resch_task_t *);
};

/**
 * task control block. 
 */
struct resch_task_struct {
	/* pointer to the container task. */
	struct task_struct *task; 
	/* resch ID. */
	int rid;
	/* real-time prio. */
	int prio; 
	/* CPU ID upon which the task is currently running. */
	int cpu_id;
	/* CPU mask upon which the task is allowed to run. */
	cpumask_t cpumask;
	/* migration flags. */
	int migratory;
	/* deadline miss flag. */
	int missed;
	/* permission for updating the WCET. */
	int wcet_fixed;
	/* timing properties: */
	unsigned long wcet; 			/* by microseconds */
	unsigned long runtime; 			/* by microseconds */
	unsigned long period; 			/* by jiffies */
	unsigned long deadline;			/* by jiffies */
	unsigned long deadline_time;	/* by jiffies */
	unsigned long release_time;		/* by jiffies */
#ifdef RESCH_HRTIMER
	struct hrtimer hrtimer_period;
#endif
#ifdef RESCH_PREEMPT_TRACE
	/* job execution monitoring. */
	unsigned long exec_time;		/* by jiffies */
	unsigned long exec_delta;		/* by jiffies */
	/* preempting & preempted tasks. */
	struct resch_task_struct *preempter;
	struct resch_task_struct *preemptee;
#endif
	/* priority is changed through kernel threads. */
	struct setscheduler_struct setsched;
	struct list_head prio_entry;
	/* we have two types of task lists. */
	struct list_head global_entry;
	struct list_head active_entry;
	/* scheduling class & policy. */
	const struct resch_sched_class *class;
	int policy;
	/* resource reservation properties. */
	int accounting;
	int xcpu;
	int prio_save; /* holds the original priority. */
	unsigned long reserve_time;		/* by jiffies */
	unsigned long budget;			/* by jiffies */
	struct timer_list replenish_timer;
#ifdef RESCH_PREEMPT_TRACE
	struct timer_list expire_timer;
#endif
	/* aperiodic server. */
	struct server_struct *server;
	/* compositional properties. */
	struct list_head component_entry;
#ifdef RESCH_HRTIMER
	int static_prio_save;
#endif
	/* for sched_deadline */
	unsigned long long *dl_runtime; /* by ns. remaining runtime for this instance. */
	unsigned long long *dl_period; /* by ns.  */
	unsigned long long *dl_deadline; /*by ns. absolute deadline for this instance  */
	unsigned long long *rq_clock; /* by ns. runqueue current clock.  */
	unsigned long long *dl_bw;
	unsigned long long dl_sched_release_time;
};

#ifdef RESCH_PREEMPT_TRACE
static inline unsigned long exec_time(resch_task_t *rt)
{
	return rt->exec_time; /* jiffies */
}
static inline unsigned long exec_delta(resch_task_t *rt)
{
	return rt->exec_delta; /* jiffies */
}
static inline void reset_exec_time(resch_task_t *rt)
{
	rt->exec_time = 0;
	rt->exec_delta = 0;
}
#else /* !RESCH_PREEMPT_TRACE */
static inline unsigned long exec_time(resch_task_t *rt)
{
	return usecs_to_jiffies((rt->task->utime + rt->task->stime)/1000);
}
static inline unsigned long exec_delta(resch_task_t *rt)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
	return usecs_to_jiffies(((rt->task->utime + rt->task->stime) - 
		(rt->task->prev_cputime.utime + rt->task->prev_cputime.stime))/1000);
#else
	return usecs_to_jiffies(((rt->task->utime + rt->task->stime) - 
		(rt->task->prev_utime + rt->task->prev_stime))/1000);
#endif
}
static inline void reset_exec_time(resch_task_t *rt)
{
	rt->task->utime = 0;
	rt->task->stime = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
	rt->task->prev_cputime.utime = 0;
	rt->task->prev_cputime.stime = 0;
#else
	rt->task->prev_utime = 0;
	rt->task->prev_stime = 0;
#endif
}
#endif

/**
 * exported global variables.
 */
extern unsigned long switch_cost;
extern unsigned long release_cost;
extern unsigned long migration_cost;

/**
 * exported functions. 
 */
extern void install_scheduler(void (*)(resch_task_t *),
							  void (*)(resch_task_t *),
							  void (*)(resch_task_t *),
							  void (*)(resch_task_t *));
extern void uninstall_scheduler(void);
extern int set_scheduler(resch_task_t *, int, int);
extern resch_task_t* get_current_task(int);
extern resch_task_t* active_highest_prio_task(int);
extern resch_task_t* active_next_prio_task(resch_task_t *);
extern resch_task_t* active_prev_prio_task(resch_task_t *);
extern resch_task_t* active_prio_task(int, int);
extern resch_task_t* active_number_task(int, int);
extern resch_task_t* global_highest_prio_task(void);
extern resch_task_t* global_next_prio_task(resch_task_t *);
extern resch_task_t* global_prev_prio_task(resch_task_t *);
extern resch_task_t* global_prio_task(int);
extern resch_task_t* global_number_task(int);
extern void active_queue_lock(int, unsigned long*);
extern void active_queue_unlock(int, unsigned long*);
extern void active_queue_double_lock(int, int, unsigned long*);
extern void active_queue_double_unlock(int, int, unsigned long*);
extern void global_list_down(void);
extern void global_list_up(void);
extern void migrate_task(resch_task_t *, int);
extern unsigned long sched_overhead_cpu(int, unsigned long);
extern unsigned long sched_overhead_task(resch_task_t *, unsigned long);

#endif
