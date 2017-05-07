/*
 * preempt_trace.c		Copyright(C) Shinpei Kato
 *
 * We trace who preempts whom, if requested.
 */

#ifdef RESCH_PREEMPT_TRACE

#include "sched.h"

/**
 * update the given CPU's tick, and also trace the execution time of 
 * the current task on the CPU, if any.
 */
static void update_tick(int cpu)
{
	unsigned long delta;
	if (lo[cpu].current_task) {
		/* the RESCH core traces the execution time by itself.
		   this is because the execution times traced in the Linux
		   kernel, such as utime and se.sum_exec_runtime, are not
		   precise when user applications are I/O intensive, due
		   to loss of scheduler_tick() invocations. */
		delta = jiffies - lo[cpu].last_tick;
		lo[cpu].current_task->exec_delta = delta;
		lo[cpu].current_task->exec_time += delta;
	}
	lo[cpu].last_tick = jiffies;
}

/**
 * lock preemption on the given CPU.
 * interrupts are also saved and disabled.
 */
static inline void preempt_lock(int cpu, unsigned long *flags)
{
	spin_lock_irqsave(&lo[cpu].preempt_lock, *flags);
}

/**
 * unlock preemption on the given CPU.
 * interrupts are also restored.
 */
static inline void preempt_unlock(int cpu, unsigned long *flags)
{
	spin_unlock_irqrestore(&lo[cpu].preempt_lock, *flags);
}

/**
 * trace preemption when the current task is preempted.
 * @lo[rt->cpu_id].current_task must be a valid reference.
 */
static inline void __preempt_current(resch_task_t *rt)
{
	rt->preemptee = lo[rt->cpu_id].current_task;
	rt->preemptee->preempter = rt;
	lo[rt->cpu_id].current_task = rt;
}

/**
 * trace preemption when idle (non real-time task) is preempted.
 */
static inline void __preempt_idle(resch_task_t *rt)
{
	rt->preemptee = NULL;
	lo[rt->cpu_id].current_task = rt;
}

/**
 * trace preemption when a new job of the given task is started.
 * we must be aware of priority inversion by self-suspention.
 */
void preempt(resch_task_t *rt)
{
	unsigned long flags;
	int cpu = rt->cpu_id;
	resch_task_t *curr = lo[cpu].current_task;

	preempt_lock(cpu, &flags);
	update_tick(cpu);
	/* we must compare the priorities because a lower-priority task may
	   be executed ahead of a higher one due to its self-suspention. */
	if (curr && curr->prio < rt->prio) {
		if (task_has_reserve(curr)) {
			preempt_account(curr);
		}
		__preempt_current(rt);
	}	
	else if (!curr) {
		__preempt_idle(rt);
	}

	if(task_has_reserve(rt) && job_is_started(rt)) {
		resume_account(rt);
	}
	preempt_unlock(cpu, &flags);
}

/**
 * trace preemption when a job of the given task is completed.
 * we must be aware of priority inversion by self-suspention.
 */
void preempt_self(resch_task_t *rt)
{
	unsigned long flags;
	int cpu = rt->cpu_id;
	resch_task_t *curr = rt->preemptee;

	preempt_lock(cpu, &flags);
	update_tick(cpu);

	if (task_has_reserve(rt)) {
		suspend_account(rt);
	}

	/* note that rt->preemptee will be not necessarily scheduled next, 
	   if some higher-priority tasks have released jobs. in that case, 
	   this task will be preempted again. */
	if (curr) {
		curr->preempter = NULL;
		if (task_has_reserve(curr)) {
			resume_account(curr);
		}
	}
	lo[cpu].current_task = curr;
	rt->preemptee = NULL;

	preempt_unlock(cpu, &flags);
}

/**
 * trace preemption when the given task moves out the current CPU.
 */
void preempt_out(resch_task_t *rt)
{
	unsigned long flags;
	int cpu = rt->cpu_id;
	resch_task_t *curr = lo[cpu].current_task;

	if (curr == rt) {
		preempt_self(rt);
	}
	else {
		preempt_lock(cpu, &flags);
		if (rt->preempter) {
			rt->preempter->preemptee = rt->preemptee;
		}
		if (rt->preemptee) {
			rt->preemptee->preempter = rt->preempter;
		}
		rt->preemptee = NULL;
		rt->preempter = NULL;
		preempt_unlock(cpu, &flags);
	}
}

/**
 * trace preemption when the given task moves in a different CPU.
 * the active queue must be locked.
 */
void preempt_in(resch_task_t *rt)
{
	unsigned long flags;
	int cpu = rt->cpu_id;
	resch_task_t *curr = lo[cpu].current_task;

	if (curr && curr->prio >= rt->prio) {
		preempt_lock(cpu, &flags);
		/* the previous task in the active queue must be valid. */
		rt->preempter = active_prev_prio_task(rt);
		rt->preemptee = rt->preempter->preemptee;
		rt->preempter->preemptee = rt;
		if (rt->preemptee) {
			rt->preemptee->preempter = rt;
		}
		preempt_unlock(cpu, &flags);
	}
	else {
		preempt(rt);
	}
}

#endif
