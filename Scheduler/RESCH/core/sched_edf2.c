/*
 * sched_edf.c		Copyright (C) Shinpei Kato
 *
 * EDF scheduler implementation for RESCH.
 */

#include <resch-core.h>
#include "sched.h"

/**
 * return the next task according to the EDF policy.
 * the active queue must be locked. 
 */
static inline resch_task_t* find_next_edf_task(int cpu)
{
	/* since tasks are stored in each queue in order of deadlines,
	   we just return the first entry. */
	return active_prio_task(cpu, RESCH_PRIO_EDF);
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

/**
 * release a new job of the given task.
 * the release time of the new job has been already set. 
 * we give the top priority to the given task if its deadline is earlier 
 * than the current task on this CPU.
 * note that we need to decrease the priority of the given task if its
 * deadline is not the earliest, since edf_job_completion() does not
 * decrease its priority to avoid the task being preempted before calling
 * the sleeping function.
 */
static void edf_job_release(resch_task_t *rt)
{
	unsigned long flags;
	resch_task_t *edf_task;
	int cpu = rt->cpu_id;

	active_queue_lock(cpu, &flags);
	edf_task = active_prio_task(cpu, RESCH_PRIO_EDF_RUN);
	if (!edf_task) {
		edf_enqueue_task(rt, RESCH_PRIO_EDF_RUN, cpu);
		active_queue_unlock(cpu, &flags);
		if (rt->task != current) {
			if (rt->prio != RESCH_PRIO_EDF_RUN) {
				request_change_prio_interrupt(rt, RESCH_PRIO_EDF_RUN);
			}
		}
		else {
			if (rt->prio != RESCH_PRIO_EDF_RUN) {
				request_change_prio(rt, RESCH_PRIO_EDF_RUN);
			}
		}
	}
	else if (rt->deadline_time < edf_task->deadline_time) {
		edf_enqueue_task(rt, RESCH_PRIO_EDF_RUN, cpu);
		active_queue_unlock(cpu, &flags);
		if (rt->task != current) {
			if (rt->prio != RESCH_PRIO_EDF_RUN) {
				request_change_prio_interrupt(rt, RESCH_PRIO_EDF_RUN);
			}
			request_change_prio_interrupt(edf_task, RESCH_PRIO_EDF);
		}
		else {
			if (rt->prio != RESCH_PRIO_EDF_RUN) {
				request_change_prio(rt, RESCH_PRIO_EDF_RUN);
			}
			request_change_prio(edf_task, RESCH_PRIO_EDF);
		}
	}
	else {
		edf_enqueue_task(rt, RESCH_PRIO_EDF, cpu);
		active_queue_unlock(cpu, &flags);
		if (rt->task != current) {
			if (rt->prio != RESCH_PRIO_EDF) {
				request_change_prio_interrupt(rt, RESCH_PRIO_EDF);
			}
		}
		else {
			if (rt->prio != RESCH_PRIO_EDF) {
				request_change_prio(rt, RESCH_PRIO_EDF);
			}
		}
	}
}

/**
 * complete the current job of the given task.
 */
static void edf_job_complete(resch_task_t *rt)
{
	unsigned long flags;
	resch_task_t *next;

	active_queue_lock(rt->cpu_id, &flags);
	next = find_next_edf_task(rt->cpu_id);
	edf_dequeue_task(rt, rt->prio, rt->cpu_id);
	active_queue_unlock(rt->cpu_id, &flags);

	/* change the priority of the next task. 
	   the next task will never preempt the current task. */
	if (next && next->prio != RESCH_PRIO_EDF_RUN) {
		request_change_prio(next, RESCH_PRIO_EDF_RUN);
	}
}

/**
 * migrate @rt to the given CPU. 
 */
static void edf_migrate_task(resch_task_t *rt, int cpu_dst)
{
	resch_task_t *next_src = NULL, *curr_dst = NULL;
	unsigned long flags;
	int cpu_src = rt->cpu_id;

	if (cpu_src != cpu_dst) {
		active_queue_double_lock(cpu_src, cpu_dst, &flags);
		if (task_is_active(rt)) {
#ifdef NO_LINUX_LOAD_BALANCE
			/* trace preemption. */
			preempt_out(rt);
#endif
			/* move off the source CPU. */
			edf_dequeue_task(rt, rt->prio, cpu_src);

			/* save the next task on the source CPU and the current task
			   on the destination CPU. */
			if (rt->prio == RESCH_PRIO_EDF_RUN) {
				next_src = find_next_edf_task(cpu_src);
			}
			curr_dst = active_prio_task(cpu_dst, RESCH_PRIO_EDF_RUN);

			/* move on the destination CPU. */
			rt->cpu_id = cpu_dst; 
			edf_enqueue_task(rt, rt->prio, cpu_dst);

#ifdef NO_LINUX_LOAD_BALANCE
			/* trace preemption. */
			preempt_in(rt);
#endif
			active_queue_double_unlock(cpu_src, cpu_dst, &flags);

			__migrate_task(rt, cpu_dst);

			/* the next task will never preempt the current task. */
			if (next_src && next_src->prio != RESCH_PRIO_EDF_RUN) {
				set_priority(next_src, RESCH_PRIO_EDF_RUN);
			}

			if (!curr_dst) {
				if (rt->prio != RESCH_PRIO_EDF_RUN) {
					set_priority(rt, RESCH_PRIO_EDF_RUN);
				}
			}
			else if (rt->deadline_time < curr_dst->deadline_time) {
				if (rt->prio != RESCH_PRIO_EDF_RUN) {
					set_priority(rt, RESCH_PRIO_EDF_RUN);
				}
				if (curr_dst->prio != RESCH_PRIO_EDF) {
					set_priority(curr_dst, RESCH_PRIO_EDF);
				}
			}
			else {
				if (rt->prio != RESCH_PRIO_EDF) {
					set_priority(rt, RESCH_PRIO_EDF);
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

/* EDF scheduling class. */
static const struct resch_sched_class edf_sched_class = {
	.enqueue_task		= edf_enqueue_task,
	.dequeue_task		= edf_dequeue_task,
	.job_release		= edf_job_release,
	.job_complete		= edf_job_complete,
	.migrate_task		= edf_migrate_task,
};
