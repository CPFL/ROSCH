/*
 * sched_fair.c		Copyright (C) Shinpei Kato
 *
 * The scheduler is subject to the Linux CFS.
 */

static int fair_set_scheduler(resch_task_t *rt, int prio)
{
	struct sched_param sp;
    sp.sched_priority = 0;
    if (sched_setscheduler(rt->task, SCHED_NORMAL, &sp) < 0) {
        printk(KERN_WARNING "RESCH: fair_change_prio() failed.\n");
        printk(KERN_WARNING "RESCH: task#%d (process#%d) priority=%d.\n",
               rt->rid, rt->task->pid, prio);
        return false;
    }

	rt->prio = prio;
	return true;
}

static void fair_enqueue_task(resch_task_t *rt, int prio, int cpu)
{
}

static void fair_dequeue_task(resch_task_t *rt, int prio, int cpu)
{
}

static void fair_job_start(resch_task_t *rt)
{
}

static void fair_job_complete(resch_task_t *rt)
{
}

static void fair_migrate_task(resch_task_t *rt, int cpu_dst)
{
}

static void fair_reserve_expire(resch_task_t *rt)
{
}

static void fair_reserve_replenish(resch_task_t *rt, unsigned long cputime)
{
}

static void fair_start_account(resch_task_t *rt)
{
}

static void fair_stop_account(resch_task_t *rt)
{
}

static void fair_wait_period(resch_task_t *rt)
{
	if (rt->release_time > jiffies) {
		rt->task->state = TASK_UNINTERRUPTIBLE;
		sleep_interval(rt, rt->release_time - jiffies);
	}
	else {
		schedule();
	}
}

static const struct resch_sched_class fair_sched_class = {
	.set_scheduler		= fair_set_scheduler,
	.enqueue_task		= fair_enqueue_task,
	.dequeue_task		= fair_dequeue_task,
	.job_start			= fair_job_start,
	.job_complete		= fair_job_complete,
	.migrate_task		= fair_migrate_task,
	.start_account		= fair_start_account,
	.stop_account		= fair_stop_account,
	.reserve_expire		= fair_reserve_expire,
	.reserve_replenish	= fair_reserve_replenish,
	.wait_period		= fair_wait_period,
};
