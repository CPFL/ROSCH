#ifndef __RESCH_SCHED_DL_H__
#define __RESCH_SCHED_DL_H__

#include <linux/hrtimer.h>
#include <linux/sched/rt.h>
#include <linux/sched/deadline.h>
#include <linux/spinlock.h>
#include <linux/stop_machine.h>
#include <linux/tick.h>
#include <linux/slab.h>

/*
 * Single value that decides SCHED_DEADLINE internal math precision.
 * 10 -> just above 1us
 * 9  -> just above 0.5us
 */
#define DL_SCALE (10)

/* XXX */
#define RQ_ERROR 0xD80

#define DL_NEW			0x00000001

/* prototype  */
struct rq;
struct cfs_rq;
struct rt_rq;
struct dl_rq;

extern __read_mostly int scheduler_running;

extern unsigned long calc_load_update;
extern atomic_long_t calc_load_tasks;

extern long calc_load_fold_active(struct rq *this_rq);
extern void update_cpu_load_active(struct rq *this_rq);

DEFINE_PER_CPU_SECTION(struct rq, runqueues, PER_CPU_SHARED_ALIGNED_SECTION)____cacheline_aligned_in_smp;

#define cpu_rq(cpu)		(&per_cpu(runqueues, (cpu)))
#define this_rq()		(&__get_cpu_var(runqueues))
#define task_rq(p)		cpu_rq(task_cpu(p))
#define cpu_curr(cpu)		(cpu_rq(cpu)->curr)
#define raw_rq()		(&__raw_get_cpu_var(runqueues))

int sched_setattr(struct task_struct *p, const struct sched_attr *attr);

/*
 * kernel variable
 */

/* CFS-related fields in a runqueue */
struct cfs_rq {
	struct load_weight load;
	unsigned int nr_running, h_nr_running;

	u64 exec_clock;
	u64 min_vruntime;
#ifndef CONFIG_64BIT
	u64 min_vruntime_copy;
#endif

	struct rb_root tasks_timeline;
	struct rb_node *rb_leftmost;

	/*
	 * 'curr' points to currently running entity on this cfs_rq.
	 * It is set to NULL otherwise (i.e when none are currently running).
	 */
	struct sched_entity *curr, *next, *last, *skip;

#ifdef	CONFIG_SCHED_DEBUG
	unsigned int nr_spread_over;
#endif

#ifdef CONFIG_SMP
	/*
	 * CFS Load tracking
	 * Under CFS, load is tracked on a per-entity basis and aggregated up.
	 * This allows for the description of both thread and group usage (in
	 * the FAIR_GROUP_SCHED case).
	 */
	unsigned long runnable_load_avg, blocked_load_avg;
	atomic64_t decay_counter;
	u64 last_decay;
	atomic_long_t removed_load;

#ifdef CONFIG_FAIR_GROUP_SCHED
	/* Required to track per-cpu representation of a task_group */
	u32 tg_runnable_contrib;
	unsigned long tg_load_contrib;

	/*
	 *   h_load = weight * f(tg)
	 *
	 * Where f(tg) is the recursive weight fraction assigned to
	 * this group.
	 */
	unsigned long h_load;
	u64 last_h_load_update;
	struct sched_entity *h_load_next;
#endif /* CONFIG_FAIR_GROUP_SCHED */
#endif /* CONFIG_SMP */

#ifdef CONFIG_FAIR_GROUP_SCHED
	struct rq *rq;	/* cpu runqueue to which this cfs_rq is attached */

	/*
	 * leaf cfs_rqs are those that hold tasks (lowest schedulable entity in
	 * a hierarchy). Non-leaf lrqs hold other higher schedulable entities
	 * (like users, containers etc.)
	 *
	 * leaf_cfs_rq_list ties together list of leaf cfs_rq's in a cpu. This
	 * list is used during load balance.
	 */
	int on_list;
	struct list_head leaf_cfs_rq_list;
	struct task_group *tg;	/* group that "owns" this runqueue */

#ifdef CONFIG_CFS_BANDWIDTH
	int runtime_enabled;
	u64 runtime_expires;
	s64 runtime_remaining;

	u64 throttled_clock, throttled_clock_task;
	u64 throttled_clock_task_time;
	int throttled, throttle_count;
	struct list_head throttled_list;
#endif /* CONFIG_CFS_BANDWIDTH */
#endif /* CONFIG_FAIR_GROUP_SCHED */
};

/*
 * This is the priority-queue data structure of the RT scheduling class:
 */
struct rt_prio_array {
	DECLARE_BITMAP(bitmap, MAX_RT_PRIO+1); /* include 1 bit for delimiter */
	struct list_head queue[MAX_RT_PRIO];
};

/* Real-Time classes' related field in a runqueue: */
struct rt_rq {
	struct rt_prio_array active;
	unsigned int rt_nr_running;
#if defined CONFIG_SMP || defined CONFIG_RT_GROUP_SCHED
	struct {
		int curr; /* highest queued rt task prio */
#ifdef CONFIG_SMP
		int next; /* next highest */
#endif
	} highest_prio;
#endif
#ifdef CONFIG_SMP
	unsigned long rt_nr_migratory;
	unsigned long rt_nr_total;
	int overloaded;
	struct plist_head pushable_tasks;
#endif
	int rt_queued;

	int rt_throttled;
	u64 rt_time;
	u64 rt_runtime;
	/* Nests inside the rq lock: */
	raw_spinlock_t rt_runtime_lock;

#ifdef CONFIG_RT_GROUP_SCHED
	unsigned long rt_nr_boosted;

	struct rq *rq;
	struct task_group *tg;
#endif
};

/* Deadline class' related fields in a runqueue */
struct dl_rq {
	/* runqueue is an rbtree, ordered by deadline */
	struct rb_root rb_root;
	struct rb_node *rb_leftmost;

	unsigned long dl_nr_running;

#ifdef CONFIG_SMP
	/*
	 * Deadline values of the currently executing and the
	 * earliest ready task on this rq. Caching these facilitates
	 * the decision wether or not a ready but not running task
	 * should migrate somewhere else.
	 */
	struct {
		u64 curr;
		u64 next;
	} earliest_dl;

	unsigned long dl_nr_migratory;
	int overloaded;

	/*
	 * Tasks on this rq that can be pushed away. They are kept in
	 * an rb-tree, ordered by tasks' deadlines, with caching
	 * of the leftmost (earliest deadline) element.
	 */
	struct rb_root pushable_dl_tasks_root;
	struct rb_node *pushable_dl_tasks_leftmost;
#else
	struct dl_bw dl_bw;
#endif
};

/*
 * This is the main, per-CPU runqueue data structure.
 *
 * Locking rule: those places that want to lock multiple runqueues
 * (such as the load balancing or the thread migration code), lock
 * acquire operations must be ordered by ascending &runqueue.
 */
struct rq {
	/* runqueue lock: */
	raw_spinlock_t lock;

	/*
	 * nr_running and cpu_load should be in the same cacheline because
	 * remote CPUs use both these fields when doing load calculation.
	 */
	unsigned int nr_running;
#ifdef CONFIG_NUMA_BALANCING
	unsigned int nr_numa_running;
	unsigned int nr_preferred_running;
#endif
	#define CPU_LOAD_IDX_MAX 5
	unsigned long cpu_load[CPU_LOAD_IDX_MAX];
	unsigned long last_load_update_tick;
#ifdef CONFIG_NO_HZ_COMMON
	u64 nohz_stamp;
	unsigned long nohz_flags;
#endif
#ifdef CONFIG_NO_HZ_FULL
	unsigned long last_sched_tick;
#endif
	int skip_clock_update;

	/* capture load from *all* tasks on this cpu: */
	struct load_weight load;
	unsigned long nr_load_updates;
	u64 nr_switches;

	struct cfs_rq cfs;
	struct rt_rq rt;
	struct dl_rq dl;

#ifdef CONFIG_FAIR_GROUP_SCHED
	/* list of leaf cfs_rq on this cpu: */
	struct list_head leaf_cfs_rq_list;

	struct sched_avg avg;
#endif /* CONFIG_FAIR_GROUP_SCHED */

	/*
	 * This is part of a global counter where only the total sum
	 * over all CPUs matters. A task can increase this counter on
	 * one CPU and if it got migrated afterwards it may decrease
	 * it on another CPU. Always updated under the runqueue lock:
	 */
	unsigned long nr_uninterruptible;

	struct task_struct *curr, *idle, *stop;
	unsigned long next_balance;
	struct mm_struct *prev_mm;

	u64 clock;
	u64 clock_task;

	atomic_t nr_iowait;

#ifdef CONFIG_SMP
	struct root_domain *rd;
	struct sched_domain *sd;

	unsigned long cpu_capacity;

	unsigned char idle_balance;
	/* For active balancing */
	int post_schedule;
	int active_balance;
	int push_cpu;
	struct cpu_stop_work active_balance_work;
	/* cpu of this runqueue: */
	int cpu;
	int online;

	struct list_head cfs_tasks;

	u64 rt_avg;
	u64 age_stamp;
	u64 idle_stamp;
	u64 avg_idle;

	/* This is used to determine avg_idle's max value */
	u64 max_idle_balance_cost;
#endif

#ifdef CONFIG_IRQ_TIME_ACCOUNTING
	u64 prev_irq_time;
#endif
#ifdef CONFIG_PARAVIRT
	u64 prev_steal_time;
#endif
#ifdef CONFIG_PARAVIRT_TIME_ACCOUNTING
	u64 prev_steal_time_rq;
#endif

	/* calc_load related fields */
	unsigned long calc_load_update;
	long calc_load_active;

#ifdef CONFIG_SCHED_HRTICK
#ifdef CONFIG_SMP
	int hrtick_csd_pending;
	struct call_single_data hrtick_csd;
#endif
	struct hrtimer hrtick_timer;
#endif

#ifdef CONFIG_SCHEDSTATS
	/* latency stats */
	struct sched_info rq_sched_info;
	unsigned long long rq_cpu_time;
	/* could above be rq->cfs_rq.exec_clock + rq->rt_rq.rt_runtime ? */

	/* sys_sched_yield() stats */
	unsigned int yld_count;

	/* schedule() stats */
	unsigned int sched_count;
	unsigned int sched_goidle;

	/* try_to_wake_up() stats */
	unsigned int ttwu_count;
	unsigned int ttwu_local;
#endif

#ifdef CONFIG_SMP
	struct llist_head wake_list;
#endif
};

/*
 *  
 */

static inline void *get_rq_of_task(struct task_struct *p)
{
   return (void*)task_rq(p) - RQ_ERROR;
}

static inline struct task_struct *dl_task_of(struct sched_dl_entity *dl_se)
{
    return container_of(dl_se, struct task_struct, dl);
}

static inline struct rq *rq_of_dl_rq(struct dl_rq *dl_rq)
{
    return container_of(dl_rq, struct rq, dl);
}

static inline struct dl_rq *dl_rq_of_se(struct sched_dl_entity *dl_se)
{
    struct task_struct *p = dl_task_of(dl_se);
    struct rq *rq = task_rq(p);

    return &rq->dl;
}

#endif
