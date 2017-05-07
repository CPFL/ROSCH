/*
 * sched_deadline.c		Copyright (C) Shinpei Kato, Yusuke Fujii
 *
 *
 * EDF scheduler implementation for RESCH.
 * This implementation is used when SCHED_DEADLINE is enabled.
 */

#include <resch-core.h>
#include "sched.h"
#include "sched_deadline.h"
#include <linux/slab.h>
#ifndef CONFIG_AIRS
#define SCHED_FCBS 		0x20000000
#define SCHED_FCBS_NO_CATCH_UP	0x10000000
#define SCHED_EXHAUSTIVE 	0x08000000
#endif

static inline uint64_t timespec_to_ms(struct timespec *ts) {
	return ((ts->tv_sec * 1000L) + (ts->tv_nsec / 1000000L ));
}

static inline uint64_t timespec_to_us(struct timespec *ts) {
	return ((ts->tv_sec * 1000000L) + (ts->tv_nsec / 1000L ));
}


int sched_wait_interval(int flags, const struct timespec __user * rqtp, struct timespec __user * rmtp){
 	struct hrtimer_sleeper t;
	enum hrtimer_mode mode = flags & TIMER_ABSTIME ?
	    HRTIMER_MODE_ABS : HRTIMER_MODE_REL;
	int ret = 0;

	hrtimer_init_on_stack(&t.timer, CLOCK_MONOTONIC, mode);
	hrtimer_set_expires(&t.timer, timespec_to_ktime(*rqtp));
	hrtimer_init_sleeper(&t, current);

	do{
	    set_current_state(TASK_INTERRUPTIBLE);
	    hrtimer_start_expires(&t.timer, mode);
	    if (!hrtimer_active(&t.timer))
		t.task = NULL;

	    if (likely(t.task)) {
		t.task->dl.flags |= DL_NEW;
		schedule();
	    }
	    hrtimer_cancel(&t.timer);
	    mode = HRTIMER_MODE_ABS;
	} while (t.task && !signal_pending(current));
	__set_current_state(TASK_RUNNING);

	if (t.task == NULL)
	    goto out;

	if (mode == HRTIMER_MODE_ABS) {
	    ret = -ERESTARTNOHAND;
	    goto out;
	}

	if (rmtp) {
	    ktime_t rmt;
	    struct timespec rmt_ts;
	    rmt = hrtimer_expires_remaining(&t.timer);
	    if (rmt.tv64 > 0)
		goto out;
	    rmt_ts = ktime_to_timespec(rmt);
	    if (!timespec_valid(&rmt_ts))
		goto out;
	    *rmtp = rmt_ts;
	}
out:
	destroy_hrtimer_on_stack(&t.timer);
	return ret;
}

/**
 * set the scheduler internally in the Linux kernel.
 */
static int edf_set_scheduler(resch_task_t *rt, int prio)
{
	uint64_t runtime, period, deadline;
	struct sched_attr sa;
	int retval;
	
	struct sched_dl_entity *dl_se = &rt->task->dl;
	struct task_struct *task = container_of(dl_se, struct task_struct, dl);
	struct dl_rq *dl_rq = dl_rq_of_se(dl_se);
	struct rq *rq = get_rq_of_task(task);

	rt->dl_runtime = &dl_se->runtime;
	rt->dl_deadline = &dl_se->deadline;
	rt->rq_clock = &rq->clock;
	rt->dl_sched_release_time = 0;

	memset(&sa, 0, sizeof(struct sched_attr));
	
	sa.sched_policy = SCHED_DEADLINE;
	sa.size = sizeof(struct sched_attr);
	sa.sched_flags = 0;
	sa.sched_deadline =  jiffies_to_nsecs(rt->deadline);
	sa.sched_period = jiffies_to_nsecs(rt->period);
	sa.sched_runtime = rt->runtime*1000;

	RESCH_DPRINT("@@@@sched_deadline status@@@@");
	RESCH_DPRINT("policy   = %16lu\n", sa.sched_policy);
	RESCH_DPRINT("deadline = %16lu\n", sa.sched_deadline);
	RESCH_DPRINT("period   = %16lu\n", sa.sched_period);
	RESCH_DPRINT("runtime  = %16lu\n", sa.sched_runtime);
	RESCH_DPRINT("now_time = %16lu jiffies\n",jiffies);
	RESCH_DPRINT("@@@@@@@@@@@@\n");
	if (sa.sched_runtime < 2 <<(DL_SCALE -1)) {
	    	printk(KERN_WARNING "RESCH: please check runtime. You have to set the runtime bigger than %d \n", 2<<(DL_SCALE -1));
	}

	rcu_read_lock();
	if (rt->task == NULL || (retval = sched_setattr(rt->task, &sa)<0)) {
		printk(KERN_WARNING "RESCH: edf_set_scheduler() failed.\n");
		printk(KERN_WARNING "RESCH: task#%d (process#%d) priority=%d.\n",
			   rt->rid, rt->task->pid, prio);
		return false;
	}
	rcu_read_unlock();
	
	rt->prio = prio;
	if (task_has_reserve(rt)) {
		rt->task->dl.flags &= ~SCHED_EXHAUSTIVE;
		rt->task->dl.flags |= SCHED_FCBS;
		/* you can additionally set the following flags, if wanted.
		   rt->task->dl.flags |= SCHED_FCBS_NO_CATCH_UP; */
	}
	else {
		rt->task->dl.flags |= SCHED_EXHAUSTIVE;
		rt->task->dl.flags &= ~SCHED_FCBS;
	}

	return true;
}

/**
 * insert the given task into the active queue according to the EDF policy.
 * the active queue must be locked.
 */
static void edf_enqueue_task(resch_task_t *rt, int prio, int cpu)
{
	int idx = prio_index(prio);
	struct prio_array *active = &lo[cpu].active;
	struct list_head *queue = &active->queue[idx];
	resch_task_t *p;

	if (list_empty(queue)) {
		list_add_tail(&rt->active_entry, queue);
		goto out;
	}
	else {
		list_for_each_entry(p, queue, active_entry) {
			if (rt->deadline_time < p->deadline_time) {
				/* insert @rt before @p. */
				list_add_tail(&rt->active_entry, &p->active_entry);
				goto out;
			}
		}
		list_add_tail(&rt->active_entry, queue);
	}
 out:
	__set_bit(idx, active->bitmap);
	active->nr_tasks++;
}

/**
 * remove the given task from the active queue.
 * the active queue must be locked.
 */
static void edf_dequeue_task(resch_task_t *rt, int prio, int cpu)
{
	int idx = prio_index(prio);
	struct prio_array *active = &lo[cpu].active;
	struct list_head *queue = &active->queue[idx];

	list_del_init(&rt->active_entry);
	if (list_empty(queue)) {
		__clear_bit(idx, active->bitmap);
	}
	active->nr_tasks--;
}

static void edf_job_start(resch_task_t *rt)
{
	unsigned long flags;
	int cpu = rt->cpu_id;

	active_queue_lock(cpu, &flags);
	edf_enqueue_task(rt, RESCH_PRIO_EDF, cpu);
	active_queue_unlock(cpu, &flags);
}

/**
 * complete the current job of the given task.
 */
static void edf_job_complete(resch_task_t *rt)
{
	int cpu = rt->cpu_id;
	unsigned long flags;

	active_queue_lock(cpu, &flags);
	edf_dequeue_task(rt, RESCH_PRIO_EDF, cpu);
	active_queue_unlock(cpu, &flags);
}

/**
 * start the accounting on the reserve of the given task in SCHED_DEADLINE.
 * if the task is properly requested to reserve CPU time through the API,
 * we do nothing here, since it is handled by SCHED_DEADLINE.
 * otherwise, we consider this is a request to forcefully account CPU time.
 * the latter case is useful if the kernel functions want to account CPU
 * time, but do not want to use the CBS policy.
 */
static void edf_start_account(resch_task_t *rt)
{
	/* set the budget explicitly, only if the task is not requested to
	   reserve CPU time through the API. */
	if (!task_has_reserve(rt)) {
		struct timespec ts;
		jiffies_to_timespec(rt->budget, &ts);
		rt->task->dl.runtime = timespec_to_ns(&ts);
	}

#if 0
	/* set the flag to notify applications when the budget is exhausted. */
	if (rt->xcpu) {
		rt->task->dl.flags |= SCHED_SIG_RORUN;
	}
#endif
	/* make sure to use Flexible CBS. */
	rt->task->dl.flags &= ~SCHED_EXHAUSTIVE;
	rt->task->dl.flags |= SCHED_FCBS;
}

/**
 * stop the accounting on the reserve of the given task in SCHED_DEADLINE.
 */
static void edf_stop_account(resch_task_t *rt)
{
	rt->task->dl.flags |= SCHED_EXHAUSTIVE;
	rt->task->dl.flags &= ~SCHED_FCBS;
#if 0
	if (rt->xcpu) {
		rt->task->dl.flags &= ~SCHED_SIG_RORUN;
		rt->task->dl.flags &= ~DL_RORUN;
	}
#endif
}

/**
 * we dont have to do anything for reserve expiration, since it is handled
 * by SCHED_DEADLINE.
 */
static void edf_reserve_expire(resch_task_t *rt)
{
	/* nothing to do. */
}

/**
 * we dont have to do anything for reserve replenishment, since it is handled
 * by SCHED_DEADLINE.
 */
static void edf_reserve_replenish(resch_task_t *rt, unsigned long cputime)
{
	/* nothing to do. */
}

/**
 * migrate @rt to the given CPU. 
 */
static void edf_migrate_task(resch_task_t *rt, int cpu_dst)
{
	unsigned long flags;
	int cpu_src = rt->cpu_id;

	if (cpu_src != cpu_dst) {
		active_queue_double_lock(cpu_src, cpu_dst, &flags);
		if (task_is_active(rt)) {
#ifdef RESCH_PREEMPT_TRACE
			/* trace preemption. */
			preempt_out(rt);
#endif
			/* move off the source CPU. */
			edf_dequeue_task(rt, RESCH_PRIO_EDF, cpu_src);

			/* move on the destination CPU. */
			rt->cpu_id = cpu_dst; 
			edf_enqueue_task(rt, RESCH_PRIO_EDF, cpu_dst);

#ifdef RESCH_PREEMPT_TRACE
			/* trace preemption. */
			preempt_in(rt);
#endif
			active_queue_double_unlock(cpu_src, cpu_dst, &flags);

			__migrate_task(rt, cpu_dst);

			/* restart accounting on the new CPU. */
			if (task_is_accounting(rt)) {
				edf_stop_account(rt);
				edf_start_account(rt);
			}
		}
		else {
			rt->cpu_id = cpu_dst;
			active_queue_double_unlock(cpu_src, cpu_dst, &flags);
			__migrate_task(rt, cpu_dst);
		}
	}
	else {
		__migrate_task(rt, cpu_dst);
	}
}

/**
 * wait until the next period.
 */
static void edf_wait_period(resch_task_t *rt)
{
	struct timespec ts_period;
	if (rt->release_time > jiffies) {
		jiffies_to_timespec(rt->release_time - jiffies, &ts_period);
	}
	else {
		ts_period.tv_sec = 0;
		ts_period.tv_nsec = 0;
	}

	if (rt->task->dl.flags & SCHED_EXHAUSTIVE) {
		rt->task->dl.deadline = cpu_clock(smp_processor_id());
	}
	sched_wait_interval(!TIMER_ABSTIME, &ts_period, NULL);
	rt->dl_sched_release_time = cpu_clock(smp_processor_id());

}

/* EDF scheduling class. */
static const struct resch_sched_class edf_sched_class = {
	.set_scheduler		= edf_set_scheduler,
	.enqueue_task		= edf_enqueue_task,
	.dequeue_task		= edf_dequeue_task,
	.job_start		= edf_job_start,
	.job_complete		= edf_job_complete,
	.start_account		= edf_start_account,
	.stop_account		= edf_stop_account,
	.reserve_expire		= edf_reserve_expire,
	.reserve_replenish	= edf_reserve_replenish,
	.migrate_task		= edf_migrate_task,
	.wait_period		= edf_wait_period,
};
