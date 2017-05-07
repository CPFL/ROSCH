/*
 * sched_fp.c		Copyright (C) Shinpei Kato
 *
 * Fixed-priority scheduler implementation for RESCH.
 */

/**
 * set the scheduler internally in the Linux kernel.
 */
static int fp_set_scheduler(resch_task_t *rt, int prio)
{
	struct sched_param sp;
	sp.sched_priority = prio;
    if (sched_setscheduler(rt->task, SCHED_FIFO, &sp) < 0) {
		printk(KERN_WARNING "RESCH: fp_change_prio() failed.\n");
		printk(KERN_WARNING "RESCH: task#%d (process#%d) prio=%d.\n",
			   rt->rid, rt->task->pid, prio);
		return false;
	}
	rt->prio = prio;
	return true;
}

/**
 * insert the given task into the active queue according to the priority.
 * the active queue must be locked.
 */
static void fp_enqueue_task(resch_task_t *rt, int prio, int cpu)
{
	int idx = prio_index(prio);
	struct prio_array *active = &lo[cpu].active;
	struct list_head *queue = &active->queue[idx];

	list_add_tail(&rt->active_entry, queue);
	__set_bit(idx, active->bitmap);
	active->nr_tasks++;
}

/**
 * remove the given task from the active queue.
 * the active queue must be locked.
 */
static void fp_dequeue_task(resch_task_t *rt, int prio, int cpu)
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
 * called when the given task is starting a new job.
 */
static void fp_job_start(resch_task_t *rt)
{
	unsigned long flags;
	int prio = rt->prio;
	int cpu = rt->cpu_id;

	active_queue_lock(cpu, &flags);
	fp_enqueue_task(rt, prio, cpu);
	active_queue_unlock(cpu, &flags);
}

/**
 * called when the given task is completing the current job.
 */
static void fp_job_complete(resch_task_t *rt)
{
	unsigned long flags;
	int prio = rt->prio;
	int cpu = rt->cpu_id;

	active_queue_lock(cpu, &flags);
	fp_dequeue_task(rt, prio, cpu);
	active_queue_unlock(cpu, &flags);
}

static void fp_start_account(resch_task_t *rt)
{
#ifdef NO_LINUX_LOAD_BALANCE
	setup_timer_on_stack(&rt->expire_timer, expire_handler, (unsigned long)rt);
	mod_timer(&rt->expire_timer, jiffies + rt->budget - exec_time(rt));	
#else
	rt->task->rt.timeout = 0;
	rt->task->signal->rlim[RLIMIT_RTTIME].rlim_cur = 
		jiffies_to_usecs(rt->budget - exec_time(rt));
#endif
	rt->prio_save = rt->prio;
}

static void fp_stop_account(resch_task_t *rt)
{
#ifdef NO_LINUX_LOAD_BALANCE
	del_timer_sync(&rt->expire_timer);
	destroy_timer_on_stack(&rt->expire_timer);
#else
	rt->task->signal->rlim[RLIMIT_RTTIME].rlim_cur = RLIM_INFINITY;
#endif
}

static void fp_reserve_expire(resch_task_t *rt)
{
	/* directly call __set_scheduler(), since the priority is decreased. */
	__set_scheduler(rt, RESCH_PRIO_BACKGROUND);
}

static void fp_reserve_replenish(resch_task_t *rt, unsigned long cputime)
{
	int cpu = rt->cpu_id;
	unsigned long flags;

	rt->budget += cputime;
	active_queue_lock(cpu, &flags);
	if (rt->prio == RESCH_PRIO_BACKGROUND && task_is_active(rt)) {
		active_queue_unlock(cpu, &flags);
		/* this can still work for the case in which the caller is
		   executing in the thread context, since in such a case the
		   caller will go to sleep soon due to periodic execution. */
		request_set_scheduler(rt, rt->prio_save);
	}
	else {
		active_queue_unlock(cpu, &flags);	
	}
}

/**
 * migrate @rt to the given CPU. 
 * @rt must be alive.
 */
static void fp_migrate_task(resch_task_t *rt, int cpu_dst)
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
			/* move the task. */			
			fp_dequeue_task(rt, rt->prio, cpu_src);
			rt->cpu_id = cpu_dst; 
			fp_enqueue_task(rt, rt->prio, cpu_dst);

#ifdef RESCH_PREEMPT_TRACE
			/* trace preemption. */
			preempt_in(rt);
#endif	
		}
		else {
			rt->cpu_id = cpu_dst;
		}
		active_queue_double_unlock(cpu_src, cpu_dst, &flags);
	}

	/* here, migrate the task. this must be done at the end of the 
	   function. otherwise the task may start execution before
	   rt->class->enqueue_task() or preempt_in() are done. */
	__migrate_task(rt, cpu_dst);

	/* restart accounting on the new CPU. */
	if (task_is_accounting(rt)) {
		fp_stop_account(rt);
		fp_start_account(rt);
	}
}

/**
 * wait until the next period.
 */
static void fp_wait_period(resch_task_t *rt)
{
	if (rt->release_time > jiffies) {
		sleep_interval(rt, rt->release_time - jiffies);
	}
	else {
		schedule();
	}
}

/* fixed-priority scheduling class. */
static const struct resch_sched_class fp_sched_class = {
	.set_scheduler		= fp_set_scheduler,
	.enqueue_task		= fp_enqueue_task,
	.dequeue_task		= fp_dequeue_task,
	.job_start			= fp_job_start,
	.job_complete		= fp_job_complete,
	.migrate_task		= fp_migrate_task,
	.reserve_replenish	= fp_reserve_replenish,
	.reserve_expire		= fp_reserve_expire,
	.start_account		= fp_start_account,
	.stop_account		= fp_stop_account,
	.wait_period		= fp_wait_period,
};
