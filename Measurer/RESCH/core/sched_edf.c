/*
 * sched_edf.c		Copyright (C) Shinpei Kato
 *
 * EDF scheduler implementation for RESCH.
 * This implementation is used when SCHED_DEADLINE is NOT supported.
 */

#include <resch-core.h>
#include "sched.h"

/**
 * set the scheduler internally in the Linux kernel.
 */
static int edf_set_scheduler(resch_task_t *rt, int prio)
{
	struct sched_param sp;
	sp.sched_priority = prio;
	if (sched_setscheduler(rt->task, SCHED_FIFO, &sp) < 0) {
		printk(KERN_WARNING "RESCH: edf_change_prio() failed.\n");
		printk(KERN_WARNING "RESCH: task#%d (process#%d) priority=%d.\n",
			   rt->rid, rt->task->pid, prio);
		return false;
	}
	
	rt->prio = prio;
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
	rt->prio = prio;
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

/**
 * called when the given task starts a new job.
 */
static void edf_job_start(resch_task_t *rt)
{
	unsigned long flags;
	int cpu = rt->cpu_id;
	resch_task_t *hp;

	active_queue_lock(cpu, &flags);

	edf_enqueue_task(rt, RESCH_PRIO_EDF, cpu);
	hp = active_highest_prio_task(cpu);
	if (rt == hp) {
		resch_task_t *curr = active_next_prio_task(rt);
		if (curr) {
			curr->task->state = TASK_INTERRUPTIBLE;
			set_tsk_need_resched(curr->task);
		}
	}
	else {
		rt->task->state = TASK_INTERRUPTIBLE;
	}

	active_queue_unlock(cpu, &flags);
}

/**
 * complete the current job of the given task.
 */
static void edf_job_complete(resch_task_t *rt)
{
	int cpu = rt->cpu_id;
	int prio = rt->prio;
	unsigned long flags;
	resch_task_t *next;

	active_queue_lock(cpu, &flags);

	edf_dequeue_task(rt, prio, cpu);
	rt->prio = RESCH_PRIO_EDF;
	next = active_highest_prio_task(cpu);

	active_queue_unlock(cpu, &flags);

	if (next) {
		wake_up_process(next->task);
	}
}

static void edf_start_account(resch_task_t *rt)
{
#ifdef NO_LINUX_LOAD_BALANCE
	setup_timer_on_stack(&rt->expire_timer, expire_handler, (unsigned long)rt);
	mod_timer(&rt->expire_timer, jiffies + rt->budget - exec_time(rt));	
#else
	rt->task->rt.timeout = 0;
	rt->task->signal->rlim[RLIMIT_RTTIME].rlim_cur = 
		jiffies_to_usecs(rt->budget - exec_time(rt));
#endif
}

static void edf_stop_account(resch_task_t *rt)
{
#ifdef NO_LINUX_LOAD_BALANCE
	del_timer_sync(&rt->expire_timer);
	destroy_timer_on_stack(&rt->expire_timer);
#else
	rt->task->signal->rlim[RLIMIT_RTTIME].rlim_cur = RLIM_INFINITY;
#endif
}

static void edf_reserve_expire(resch_task_t *rt)
{
	unsigned long flags;
	int cpu = rt->cpu_id;

	active_queue_lock(cpu, &flags);
	if (!task_is_active(rt)) {
		active_queue_unlock(cpu, &flags);
		return;
	}
	else {
		int prio = rt->prio;
		resch_task_t *next;

		/* move to the background queue. */
		edf_dequeue_task(rt, prio, cpu);
		edf_enqueue_task(rt, RESCH_PRIO_BACKGROUND, cpu);
		/* next is never NULL. */
		next = active_highest_prio_task(cpu);
		if (rt == next) {
			active_queue_unlock(cpu, &flags);
		}
		else {
			active_queue_unlock(cpu, &flags);
			wake_up_process(next->task);
			rt->task->state = TASK_INTERRUPTIBLE;
		}
	}
}

static void edf_reserve_replenish(resch_task_t *rt, unsigned long cputime)
{
	int cpu = rt->cpu_id;
	unsigned long flags;

	rt->budget += cputime;
	active_queue_lock(cpu, &flags);
	if (rt->prio == RESCH_PRIO_BACKGROUND && task_is_active(rt)) {
		edf_dequeue_task(rt, RESCH_PRIO_BACKGROUND, cpu);
		edf_enqueue_task(rt, RESCH_PRIO_EDF, cpu);
		if (rt == active_highest_prio_task(cpu) && 
			rt->task->state == TASK_INTERRUPTIBLE) {
			resch_task_t *p = active_next_prio_task(rt);
			if (p) {
				p->task->state = TASK_INTERRUPTIBLE;
			}
			active_queue_unlock(cpu, &flags);
			wake_up_process(rt->task);
		}
		else {
			active_queue_unlock(cpu, &flags);
		}
	}
	else {
		active_queue_unlock(cpu, &flags);
	}
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
			resch_task_t *next_src = NULL, *curr_dst = NULL;
			/* save the next task on the source CPU. */
			if (rt == active_highest_prio_task(cpu_src)) {
				next_src = active_next_prio_task(rt);
			}
#ifdef NO_LINUX_LOAD_BALANCE
			/* trace preemption. */
			preempt_out(rt);
#endif
			/* move off the source CPU. */
			edf_dequeue_task(rt, rt->prio, cpu_src);

			/* save the current task on the destination CPU. */
			curr_dst = active_prio_task(cpu_dst, RESCH_PRIO_EDF_RUN);

			/* move on the destination CPU. */
			rt->cpu_id = cpu_dst; 
			edf_enqueue_task(rt, rt->prio, cpu_dst);

#ifdef NO_LINUX_LOAD_BALANCE
			/* trace preemption. */
			preempt_in(rt);
#endif
			active_queue_double_unlock(cpu_src, cpu_dst, &flags);

			/* the next task will never preempt the current task. */
			if (next_src) {
				wake_up_process(next_src->task);
			}

			__migrate_task(rt, cpu_dst);

			/* restart accounting on the new CPU. */
			if (task_is_accounting(rt)) {
				edf_stop_account(rt);
				edf_start_account(rt);
			}

			if (curr_dst) {
				if (rt->deadline_time < curr_dst->deadline_time) {
					curr_dst->task->state = TASK_INTERRUPTIBLE;
					set_tsk_need_resched(curr_dst->task);
				}
				else {
					rt->task->state = TASK_INTERRUPTIBLE;
					set_tsk_need_resched(rt->task);
				}
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
	if (rt->release_time > jiffies) {
		rt->task->state = TASK_UNINTERRUPTIBLE;
		sleep_interval(rt, rt->release_time - jiffies);
	}
	else {
		schedule();
	}
}

/* EDF scheduling class. */
static const struct resch_sched_class edf_sched_class = {
	.set_scheduler		= edf_set_scheduler,
	.enqueue_task		= edf_enqueue_task,
	.dequeue_task		= edf_dequeue_task,
	.job_start			= edf_job_start,
	.job_complete		= edf_job_complete,
	.start_account		= edf_start_account,
	.stop_account		= edf_stop_account,
	.reserve_expire		= edf_reserve_expire,
	.reserve_replenish	= edf_reserve_replenish,
	.migrate_task		= edf_migrate_task,
	.wait_period		= edf_wait_period,
};
