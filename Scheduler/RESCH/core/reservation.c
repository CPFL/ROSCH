/*
 * reservation.c	Copyright (C) Shinpei Kato
 *
 * Cooperating resource reservation (CRR) and related functions.
 * We have two approaches depending on RESCH configuration.
 * If RESCH allows the Linux kernel to exploit load balance, we use
 * Linux watchdog and timeout functions to limit resource usage.
 * Otherwise, we use our own timer functions.
 */

#include <asm/current.h>
#include <linux/jiffies.h>
#include <linux/resource.h>
#include <linux/module.h>
#include <resch-api.h>
#include <resch-config.h>
#include <resch-core.h>
#include "reservation.h"
#include "sched.h"
//#if LINUX_KERNEL_MINOR_VERSION >= 34
#include <linux/slab.h>
//#endif

#ifdef RESCH_PREEMPT_TRACE
/**
 * called when the budget of a task pointed to by @__data expires in
 * resource reservation mode. 
 * note that there is no need to check out the resource here.
 */
static void expire_handler(unsigned long __data)
{
	resch_task_t *rt = (resch_task_t *)__data;
	
	if (rt->xcpu) {
		send_sig(SIGXCPU, rt->task, 0);
	}
	else {
		/* schedule the task in background. 
		   note that the CPU tick and task preemptions are traced when
		   the priority is changed in change_prio(). */
		request_change_prio_interrupt(rt, RESCH_PRIO_BACKGROUND);
	}
}
#endif

/**
 * called when a server associated with the task pointed to by @__data
 * should replenish its budget.
 */
static void server_handler(unsigned long __data)
{
	server_t *s = (server_t *)__data;
	resch_task_t *rt = s->rt;

	rt->budget += + s->capacity;

	del_timer(&s->timer);
	kfree(s);
	rt->server = NULL;
}

static void on_replenish(unsigned long __data)
{
	resch_task_t *rt = (resch_task_t*)__data;
	rt->class->reserve_replenish(rt, rt->reserve_time);
}

void reserve_replenish(resch_task_t *rt)
{
	unsigned long flags;

	local_irq_save(flags);
	if (rt->release_time + rt->period > jiffies || 
		!timer_pending(&rt->replenish_timer)) {
		rt->class->reserve_replenish(rt, rt->reserve_time);
	}
	if (timer_pending(&rt->replenish_timer)) {
		del_timer(&rt->replenish_timer);
	}
	local_irq_restore(flags);
}

/**
 * start the accounting on the reserve of the given task.
 */
void start_account(resch_task_t *rt)
{
	if (!task_is_accounting(rt)) {
		rt->budget = rt->reserve_time - min(exec_time(rt), rt->reserve_time);
		rt->class->start_account(rt);
		rt->accounting = true;
	}
}

/**
 * stop the accounting on the reserve of the given task.
 */
void stop_account(resch_task_t *rt)
{
	if (task_is_accounting(rt)) {
		rt->budget = rt->reserve_time - min(exec_time(rt), rt->reserve_time);
		rt->class->stop_account(rt);
		rt->accounting = false;
	}

	if (rt->server) {
		del_timer(&rt->server->timer);
		kfree(rt->server);
		rt->server = NULL;
	}
}

/**
 * suspend accounting on the reserve of the given task for preemption.
 */
void preempt_account(resch_task_t *rt)
{
	if (task_is_accounting(rt)) {
		stop_account(rt);
	}
}

/**
 * suspend accounting on the reserve of the given task for self suspention.
 */
void suspend_account(resch_task_t *rt)
{
	if (task_is_accounting(rt)) {
		stop_account(rt);
		if (rt->server && exec_delta(rt)) {
			rt->server->capacity = min(exec_time(rt), rt->budget);
		}
	}
	else {
		/* there can be a case in which this function is called even 
		   though the task does not hold reservation. in fact it is 
		   called every time preemption is traced in the RESCH core, 
		   and preemption is traced every time the context is switched 
		   or changes the priority. 
		   in this case, the reservation timeout should not be consumed.
		   hence we charge the time consumed. */
		rt->budget += exec_delta(rt);
	}
}

/**
 * resume accounting on the reserve of the given task.
 */
void resume_account(resch_task_t *rt)
{
	start_account(rt);
}

/**
 * start the CPU reservation for the given task.
 */
void reserve_start(resch_task_t *rt, unsigned long cputime, int xcpu)
{
	setup_timer(&rt->replenish_timer, on_replenish, (unsigned long)rt);
	rt->reserve_time = msecs_to_jiffies(cputime);
	rt->xcpu = xcpu;

	/* if the job is already started, start accounting now. */
	start_account(rt);
}

/**
 * stop the CPU reservation for the given task.
 */
void reserve_stop(resch_task_t *rt)
{
	unsigned long flags;

	local_irq_save(flags);
	if (timer_pending(&rt->replenish_timer)) {
		del_timer(&rt->replenish_timer);
	}
	
	stop_account(rt);
	rt->reserve_time = 0;
	local_irq_restore(flags);
}

/**
 * API: set up the reserve on CPU resource for the given task. 
 */
int api_reserve_start(int rid, unsigned long cputime, int xcpu)
{
	resch_task_t *rt = resch_task_ptr(rid);

	if (!task_has_reserve(rt)) {
		reserve_start(rt, cputime, xcpu);
		return RES_SUCCESS;
	}
	return RES_FAULT;
}

/**
 * API: stop accounting the CPU resource for the given task. 
 */
int api_reserve_stop(int rid)
{
	resch_task_t *rt = resch_task_ptr(rid);

	if (task_has_reserve(rt)) {
		reserve_stop(rt);
		return RES_SUCCESS;
	}
	return RES_FAULT;
}

/**
 * API: enforce calling the reserve expiration procedure.
 */
int api_reserve_expire(int rid)
{
	resch_task_t *rt = resch_task_ptr(rid);

	if (!task_has_reserve(rt)) {
		schedule();
		return RES_FAULT;
	}

	if (task_is_accounting(rt)) {
		stop_account(rt);
	}

	/* default handler for reserve expiration.
	   if the current time is later than the next expected replenishment 
	   time, immediately replenish the reserve and start accounting.
	   otherwise, run a timer so that the reserve will be relenished at 
	   the next expected replenishment time. */
	if (rt->release_time + rt->period <= jiffies) {
		rt->class->reserve_replenish(rt, rt->reserve_time);
		start_account(rt);
	}	
	else {
		rt->class->reserve_expire(rt);
		mod_timer(&rt->replenish_timer, rt->release_time + rt->period);
		schedule();
	}

	return RES_SUCCESS;
}

/**
 * API: run a server to execute the given task. 
 * It always reset the execution time of the task served.
 */
int api_server_run(int rid)
{
	server_t *s;
	resch_task_t *rt = resch_task_ptr(rid);

	s = kmalloc(sizeof(*s), GFP_KERNEL);
	if (!s) {
		printk(KERN_WARNING "RESCH: cannot allocate heap memory\n");
		return RES_FAULT;
	}

	/* sporadic server implementation. */
	setup_timer_on_stack(&s->timer, 
						 server_handler, 
						 (unsigned long)s);
	mod_timer(&s->timer, jiffies + rt->period);	

	rt->server = s;
	s->rt = rt;

	/* reset execution time. */
	rt->budget -= min(exec_time(rt), rt->budget);
	reset_exec_time(rt);

	return RES_SUCCESS;
}
