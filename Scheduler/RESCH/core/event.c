/*
 * event.c			Copyright (C) Shinpei Kato
 *
 * Sleep, suspend, and wake-up functions.
 */

#include <resch-api.h>
#include <resch-core.h>
#include "event.h"
#include "reservation.h"
#include "sched.h"

static void job_resume(resch_task_t *rt)
{
	int cpu = rt->cpu_id;
	unsigned long flags;
	active_queue_lock(cpu, &flags);
	rt->class->enqueue_task(rt, rt->prio, cpu);
#ifdef RESCH_PREEMPT_TRACE
	/* preempt_in() must be within a critical section. */
	preempt_in(rt);
#endif
	active_queue_unlock(cpu, &flags);
}

static void job_suspend(resch_task_t *rt)
{
	int cpu = rt->cpu_id;
	unsigned long flags;

	/* put back the original priority so that it can respond to wake-up. */
	if (rt->prio != rt->prio_save) {
		set_scheduler(rt, rt->policy, rt->prio_save);
	}

#ifdef RESCH_PREEMPT_TRACE
	/* preempt_self() should be out of a critical section. */
	preempt_self(rt);
#endif
	
	active_queue_lock(cpu, &flags);
	rt->class->dequeue_task(rt, rt->prio, cpu);
	active_queue_unlock(cpu, &flags);
} 

int api_sleep(int rid, unsigned long timeout)
{
	resch_task_t *rt = resch_task_ptr(rid);

	job_suspend(rt);
	sleep_interval(rt, timeout);
	job_resume(rt);

	return RES_SUCCESS;
}

int api_suspend(int rid)
{
	resch_task_t *rt = resch_task_ptr(rid);

	job_suspend(rt);
	rt->task->state = TASK_UNINTERRUPTIBLE;
	schedule();
	job_resume(rt);

	return RES_SUCCESS;
}

int api_wake_up(int rid)
{
	resch_task_t *rt = resch_task_ptr(rid);

	/* we cannot wake up tasks that are waiting for the next period. */
	if (rt->release_time > jiffies) {
		return RES_FAULT;
	}
	/* directly wake up the task. */
	if (rt->task->state != TASK_RUNNING) {
		wake_up_process(rt->task);
	}

	return RES_SUCCESS;
}

