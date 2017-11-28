/*
 * sched.c			Copyright (C) Shinpei Kato
 *
 * Basic scheduler functions of the RESCH core.
 * We must be at least able to access the following kernel functions:
 * - schedule()
 * - sched_setscheduler()
 * - wake_up_process()
 * - setup_timer_on_stack()
 * - mod_timer()
 * - del_timer_sync()
 * - destroy_timer_on_stack()
 * - send_sig()
 */

#include <asm/current.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/module.h>
#include <resch-api.h>
#include <resch-core.h>
#include "bitops.h"
#include "component.h"
#include "reservation.h"
#include "sched_ros.h"
#include "sched.h"
#include "test.h"
#include <resch-config.h>
/**
 * RESCH task descriptor.
 */
resch_task_t resch_task[NR_RT_TASKS];

/**
 * the local object for each CPU .
 * lo[0...NR_RT_CPUS-1] is corresponding to each CPU.
 * lo[NR_RT_CPUS] is prepared for such tasks that have undefined CPU ID.
 */
struct local_object lo[NR_RT_CPUS + 1];

/**
 * a bitmap for the process ID used in RESCH: RESCH ID (RID).
 * those RIDs range in [0, NR_RT_TASKS-1], which are different from
 * the process IDs used in Linux.
 * each process has a RESCH PID within the range of [0, NR_RT_TASKS-1].
 * if the k-th bit is set, the ID of k has been used for some process.
 */
struct pid_map_struct pid_map;

/**
 * a priority-ordered double-linked list, which includes all the submitted
 * tasks regardless of its running status.
 * this global list is often accessed before tasks are submitted, like
 * for response time analysis, and needs not to provide a timing-critical
 * implementation. so we use a semaphore to lock for synchronization.
 */
struct task_list_struct task_list;

/**
 * preemption cost and migration cost by microseconds (usecs).
 */
unsigned long switch_cost = 20;
unsigned long release_cost = 5;
unsigned long migration_cost = 20;

/**
 * scheduler plugins:
 * - task_run is called when the task is submitted to the system.
 * - task_exit is called when the task exit real-time execution.
 * - job_start is called when the task is starting a new job.
 * - job_complete is called when the task is completing the current job.
 */
static struct plugin_struct {
	void (*task_run)(resch_task_t*);
	void (*task_exit)(resch_task_t*);
	void (*job_start)(resch_task_t*);
	void (*job_complete)(resch_task_t*);
} sched_plugins;

#ifdef NO_LINUX_LOAD_BALANCE
/**
 * allow the given task to migrate across CPUs by the load balancing
 * function of the Linux kernel.
 */
static void enable_load_balance(resch_task_t *rt)
{
	/* restore the CPU mask. */
	cpumask_copy(&rt->task->cpus_allowed, &rt->cpumask);
}

/**
 * forbid the given task to migrate across CPUs by the load balancing
 * function of the Linux kernel
 */
static void disable_load_balance(resch_task_t *rt)
{
	cpumask_t mask;

	local_irq_disable();
	/* save the CPU mask. */
	cpumask_copy(&rt->cpumask, &rt->task->cpus_allowed);
	/* temporarily disable migration. */
#ifdef USE_VIVID_OR_OLDER
  cpus_clear(mask);
	cpu_set(smp_processor_id(), mask); 
#else
	cpumask_clear(&rt->cpumask);
	cpumask_set_cpu(smp_processor_id(), &mask);
#endif

	cpumask_copy(&rt->task->cpus_allowed, &mask);
	local_irq_enable();
}

/**
 * check if the given task has been moved to a different CPU since we
 * enabled the load balancing function of the Linux kernel.
 */
static void follow_load_balance(resch_task_t *rt)
{
	int cpu_now;
	int cpu_old;
	unsigned long flags;

	cpu_now = smp_processor_id();
	cpu_old = rt->cpu_id;
	if (cpu_now != cpu_old) {
		/* move to a correct active queue.
		   note that this task has no preemptee and preempter, since
		   its job has not been started yet. */
		active_queue_double_lock(cpu_now, cpu_old, &flags);
		rt->class->dequeue_task(rt, rt->prio, cpu_old);
		rt->cpu_id = cpu_now;
		rt->class->enqueue_task(rt, rt->prio, cpu_now);
		active_queue_double_unlock(cpu_now, cpu_old, &flags);
	}
}
#endif /* NO_LINUX_LOAD_BALANCE */

/**
 * insert the given task into the active queue, safely with lock.
 */
void enqueue_task(resch_task_t *rt, int prio, int cpu)
{
	unsigned long flags;

	active_queue_lock(cpu, &flags);
	rt->class->enqueue_task(rt, prio, cpu);
	active_queue_unlock(cpu, &flags);
}

/**
 * remove the given task from the active queue, safely with lock.
 */
void dequeue_task(resch_task_t *rt, int prio, int cpu)
{
	unsigned long flags;

	active_queue_lock(cpu, &flags);
	rt->class->dequeue_task(rt, prio, cpu);
	active_queue_unlock(cpu, &flags);
}

/**
 * reinsert the given task into the active queue, safely with lock.
 */
static void requeue_task(resch_task_t *rt, int prio_old)
{
	unsigned long flags;
	int cpu = rt->cpu_id;
	int prio_new = rt->prio;

	active_queue_lock(cpu, &flags);

#ifdef RESCH_PREEMPT_TRACE
	preempt_out(rt);
#endif

	/* requeue the task. */
	rt->class->dequeue_task(rt, prio_old, cpu);
	rt->class->enqueue_task(rt, prio_new, cpu);

#ifdef RESCH_PREEMPT_TRACE
	preempt_in(rt);
#endif

	active_queue_unlock(cpu, &flags);
}

/**
 * insert @rt into the global list in order of priority.
 * the global list must be locked.
 */
static void __global_list_insert(resch_task_t *rt)
{
	resch_task_t *p;

	/* we here insert @rt into the global list. */
	if (list_empty(&rt->global_entry)) {
		list_for_each_entry(p, &task_list.head, global_entry) {
			if (p->prio < rt->prio) {
				/* insert @rt before @p. */
				list_add_tail(&rt->global_entry, &p->global_entry);
				return; /* MUST be returned here.*/
			}
		}
		/* if the global list is empty or @rt is the lowest-priority task,
		   just insert it to the tail of the global list. */
		list_add_tail(&rt->global_entry, &task_list.head);
	}
	else {
		printk(KERN_WARNING
			   "RESCH: task#%d has been in the global list.\n", rt->rid);
	}
}

/**
 * remove @rt from the global list.
 * the global list must be locked.
 */
static void __global_list_remove(resch_task_t *rt)
{
	if (!list_empty(&rt->global_entry)) {
		list_del_init(&rt->global_entry);
	}
	else {
		printk(KERN_WARNING
			   "RESCH: task#%d has not been in the global list.\n", rt->rid);
	}
}

/**
 * insert @rt into the global list safely with spin lock.
 */
static void global_list_insert(resch_task_t *rt)
{
	global_list_down();
	__global_list_insert(rt);
	global_list_up();
}

/**
 * remove @rt from the global list safely with spin lock.
 */
static void global_list_remove(resch_task_t *rt)
{
	global_list_down();
	__global_list_remove(rt);
	global_list_up();
}

/**
 * reinsert @rt into the global list safely with spin lock.
 */
static void global_list_reinsert(resch_task_t *rt)
{
	global_list_down();
	__global_list_remove(rt);
	__global_list_insert(rt);
	global_list_up();
}

/**
 * change the priority of the given task, and save it to @rt->prio.
 */
static int __set_scheduler(resch_task_t *rt, int prio)
{
	int prio_old = rt->prio;

	/* call the scheduler internally in the Linux kernel. */
	if (!rt->class->set_scheduler(rt, prio)) {
		return false;
	}

	/* if the priority is the same, we do not touch the list and queue. */
	if (prio_old == rt->prio) {
		goto out;
	}

	/* reinsert the task into the global list, ONLY IF it has been
	   already submitted to RESCH. */
	if (task_is_managed(rt)) {
		global_list_reinsert(rt);
	}

	/* requeue the task into the active queue, ONLY IF it is active. */
	if (task_is_active(rt)) {
		requeue_task(rt, prio_old);
	}

 out:
	RESCH_DPRINT("task#%d changed policy to %d\n",
		   rt->rid, rt->task->policy);
	return true;
}

/**
 * request the setscheduler thread to change the priority of @rt.
 * the caller will sleep until the priority is changed.
 */
void request_set_scheduler(resch_task_t *rt, int prio)
{
	int cpu = smp_processor_id();

	INIT_LIST_HEAD(&rt->prio_entry);
	rt->setsched.prio = prio;
	rt->setsched.caller = current;

	/* insert the task to the waiting list for sched_setscheduler(). */
	spin_lock_irq(&lo[cpu].sched_thread.lock);
	list_add_tail(&rt->prio_entry, &lo[cpu].sched_thread.list);
	spin_unlock(&lo[cpu].sched_thread.lock);

	/* wake up the migration thread. */
	wake_up_process(lo[cpu].sched_thread.task);
	/* the task is going to sleep. */
	set_current_state(TASK_UNINTERRUPTIBLE);
	local_irq_enable();
	schedule();
}

/**
 * request the setscheduler thread to change the priority of @rt from the
 * interrupt contexts.
 * the caller will sleep until the priority is changed.
 */
void request_set_scheduler_interrupt(resch_task_t *rt, int prio)
{
	unsigned long flags;
	int cpu = smp_processor_id();

	INIT_LIST_HEAD(&rt->prio_entry);
	rt->setsched.prio = prio;
	rt->setsched.caller = current;

	/* insert the task to the waiting list for sched_setscheduler(). */
	spin_lock_irqsave(&lo[cpu].sched_thread.lock, flags);
	list_add_tail(&rt->prio_entry, &lo[cpu].sched_thread.list);
	spin_unlock(&lo[cpu].sched_thread.lock);

	/* wake up the migration thread. */
	wake_up_process(lo[cpu].sched_thread.task);
	/* the task is going to sleep. */
	set_current_state(TASK_UNINTERRUPTIBLE);
	set_tsk_need_resched(current);
	local_irq_restore(flags);
}

#ifdef RESCH_HRTIMER
static enum hrtimer_restart period_timer(struct hrtimer *timer)
{
	resch_task_t *rt = container_of(timer, resch_task_t, hrtimer_period);
	wake_up_process(rt->task);
	set_tsk_need_resched(current);
	return HRTIMER_NORESTART;
}

static int start_period_timer(resch_task_t *rt, u64 interval)
{
	hrtimer_set_expires(&rt->hrtimer_period, ns_to_ktime(interval));
	hrtimer_start_expires(&rt->hrtimer_period, HRTIMER_MODE_REL);
	return hrtimer_active(&rt->hrtimer_period);
}

static void init_period_timer(struct hrtimer *timer)
{
	hrtimer_init(timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer->function = period_timer;
}

void hrtimer_sleep_interval(resch_task_t *rt, unsigned long interval)
{
	struct timespec ts;
	u64 ns;
	jiffies_to_timespec(interval, &ts);
	ns = timespec_to_ns(&ts);
	init_period_timer(&rt->hrtimer_period);
	if (!start_period_timer(rt, ns)) {
		schedule_timeout(interval);
	}
	else {
		rt->task->state = TASK_UNINTERRUPTIBLE;
		schedule();
	}
}
#endif

/**
 * the given task will sleep until the next release time.
 */
static void job_release(resch_task_t *rt, unsigned long release_time)
{
	/* set the next release time. */
	rt->release_time = release_time;

	/* call a class function. */
	rt->class->wait_period(rt);

	/* renew the timing properties. */
	reset_exec_time(rt);
	rt->deadline_time = rt->release_time + rt->deadline;

#ifdef SCHED_DEADLINE
	if ( rt->policy == RESCH_SCHED_EDF ){
	    while(rt->runtime <=0){
		rt->deadline_time+= rt->period;
		rt->release_time = jiffies;
		rt->dl_sched_release_time = cpu_clock(smp_processor_id());
		RESCH_DPRINT("Deadline is earlier than current time. New deadline is %lu (jiffies)\n",rt->deadline_time);
	    }
	}
#endif

}

/**
 * start a new job of the given task.
 */
static void job_start(resch_task_t *rt)
{
#ifdef NO_LINUX_LOAD_BALANCE
	disable_load_balance(rt);
	follow_load_balance(rt);
#endif

#ifdef RESCH_PREEMPT_TRACE
	preempt(rt);
#endif

	/* call a class function. */
	rt->class->job_start(rt);

	/* call a plugin function. */
	sched_plugins.job_start(rt);

	/* start the accounting on CPU time, if the task has a reserve. */
	if (task_has_reserve(rt)) {
		start_account(rt);
	}

	RESCH_DPRINT("task#%d started on CPU%d at %lu (deadline=%lu).\n",
		   rt->rid, smp_processor_id(), jiffies, rt->deadline_time);
}

/**
 * complete the current job of the given task.
 */
static void job_complete(resch_task_t *rt)
{
	RESCH_DPRINT("task#%d completed on CPU%d at %lu\n",
		   rt->rid, smp_processor_id(), jiffies);
	RESCH_DPRINT("task#%d spent %lu (jiffies).\n", rt->rid, exec_time(rt));

#ifdef RESCH_PREEMPT_TRACE
	preempt_self(rt);
#endif

	/* renew the WCET if necessary. */
	if (!rt->wcet_fixed && jiffies_to_usecs(exec_time(rt)) > rt->wcet) {
		rt->wcet = jiffies_to_usecs(exec_time(rt));
	}

	/* call a plugin function. */
	sched_plugins.job_complete(rt);

	/* call a class function. */
	rt->class->job_complete(rt);

	/* replenish the reserve for the next period, if necessary. */
	if (task_has_reserve(rt)) {
		stop_account(rt);
		reserve_replenish(rt);
	}

#ifdef NO_LINUX_LOAD_BALANCE
	/* finally, enable load balancing for this task. */
	enable_load_balance(rt);
#endif
}

/**
 * check if the deadline was missed.
 */
static inline int deadline_miss(resch_task_t *rt)
{
	if (rt->deadline > 0 && rt->deadline_time < jiffies) {
		RESCH_DPRINT("task#%d missed a deadline on CPU#%d.\n",
			   rt->rid, smp_processor_id());
		RESCH_DPRINT("deadline = %lu, jiffies = %lu.\n",
			   rt->deadline_time, jiffies);
		rt->missed = true;
	}
	else {
		rt->missed = false;
	}
	return rt->missed;
}

/* include algorithm-dependent files. */
#include "sched_fair.c"
#include "sched_fp.c"
#ifdef SCHED_DEADLINE
#include "sched_deadline.c"
#else
#include "sched_edf.c"
#endif

/**
 * default function for the task_run plugin.
 */
static inline void task_run_default(resch_task_t *rt)
{
}

/**
 * default function for the task_exit plugin.
 */
static inline void task_exit_default(resch_task_t *rt)
{
}

/**
 * default function for the job_start plugin.
 */
static inline void job_start_default(resch_task_t *rt)
{
}

/**
 * default function for the job_complete plugin.
 */
static inline void job_complete_default(resch_task_t *rt)
{
}

/**
 * install the given scheduler plugins.
 */
void install_scheduler(void (*task_run_plugin)(resch_task_t*),
					   void (*task_exit_plugin)(resch_task_t*),
					   void (*job_start_plugin)(resch_task_t*),
					   void (*job_complete_plugin)(resch_task_t*))
{
	if (task_run_plugin)
		sched_plugins.task_run = task_run_plugin;
	else
		sched_plugins.task_run = task_run_default;

	if (task_exit_plugin)
		sched_plugins.task_exit = task_exit_plugin;
	else
		sched_plugins.task_exit = task_exit_default;

	if (job_start_plugin)
		sched_plugins.job_start = job_start_plugin;
	else
		sched_plugins.job_start = job_start_default;

	if (job_complete_plugin)
		sched_plugins.job_complete = job_complete_plugin;
	else
		sched_plugins.job_complete = job_complete_default;
}
EXPORT_SYMBOL(install_scheduler);

/**
 * uninstall the scheduler plugins.
 */
void uninstall_scheduler(void)
{
	sched_plugins.task_run = task_run_default;
	sched_plugins.task_exit = task_exit_default;
	sched_plugins.job_start = job_start_default;
	sched_plugins.job_complete = job_complete_default;
}
EXPORT_SYMBOL(uninstall_scheduler);

/**
 * set the new scheduling policy and priority.
 */
int set_scheduler(resch_task_t *rt, int policy, int prio)
{
	if (rt->policy != policy) {
		switch (policy) {
		case RESCH_SCHED_FP:
			rt->policy = RESCH_SCHED_FP;
			rt->class = &fp_sched_class;
			break;

		case RESCH_SCHED_EDF:
			rt->policy = RESCH_SCHED_EDF;
			rt->class = &edf_sched_class;
			break;

		case RESCH_SCHED_FAIR:
			rt->policy = RESCH_SCHED_FAIR;
			rt->class = &fair_sched_class;
			break;

		default:
			printk(KERN_WARNING "RESCH: unknown scheduler.\n");
			rt->policy = RESCH_SCHED_FAIR;
			rt->class = &fair_sched_class;
			break;
		}
	}

	if (capable(CAP_SYS_NICE)) {
		return __set_scheduler(rt, prio);
	}

	request_set_scheduler(rt, prio);
	return true;
}
EXPORT_SYMBOL(set_scheduler);

/**
 * return the currently executing task on the given CPU.
 */
resch_task_t* get_current_task(int cpu)
{
#ifdef RESCH_PREEMPT_TRACE
	return 	lo[cpu].current_task;
#else
	unsigned long flags;
	resch_task_t *curr;
	active_queue_lock(cpu, &flags);
	curr = active_highest_prio_task(cpu);
	active_queue_unlock(cpu, &flags);
	return curr;
#endif
}
EXPORT_SYMBOL(get_current_task);

/**
 * return the highest-priority task actively running on the given CPU.
 * return NULL if the CPU has no ready tasks managed by Resch.
 * the active queue must be locked.
 */
resch_task_t* active_highest_prio_task(int cpu)
{
	int idx;
	struct prio_array *active = &lo[cpu].active;

	if ((idx = resch_ffs(active->bitmap, RESCH_PRIO_LONG)) < 0) {
		return NULL;
	}
#ifdef DEBUG_PRINT
	if (list_empty(&active->queue[idx])) {
		RESCH_DPRINT("active queue may be broken on CPU#%d.\n", cpu);
		return NULL;
	}
#endif
	/* the first entry must be a valid reference due to non-negative idx. */
	return list_first_entry(&active->queue[idx],
							resch_task_t,
							active_entry);
}
EXPORT_SYMBOL(active_highest_prio_task);

/**
 * return the task positioned at the next of @rt in the active queue.
 * return NULL if there is no next task.
 * the active queue must be locked.
 */
resch_task_t* active_next_prio_task(resch_task_t *rt)
{
	int idx = prio_index(rt->prio);
	struct prio_array *active = &lo[rt->cpu_id].active;

	if (rt->active_entry.next == &(active->queue[idx])) {
		/* find next set bit with offset of idx. */
		if ((idx = resch_fns(active->bitmap, idx + 1, RESCH_PRIO_LONG)) < 0) {
			return NULL;
		}
		return list_first_entry(&active->queue[idx],
								resch_task_t,
								active_entry);
	}
	return list_entry(rt->active_entry.next,
					  resch_task_t,
					  active_entry);
}
EXPORT_SYMBOL(active_next_prio_task);

/**
 * return the task positioned at the previous of @rt in the active queue.
 * return NULL if there is no previous task.
 * the active queue must be locked.
 */
resch_task_t* active_prev_prio_task(resch_task_t *rt)
{
	int idx = prio_index(rt->prio);
	struct prio_array *active = &lo[rt->cpu_id].active;

	if (rt->active_entry.prev == &(active->queue[idx])) {
		/* find previous set bit with offset of idx. */
		if ((idx = resch_fps(active->bitmap, idx - 1, RESCH_PRIO_LONG)) < 0) {
			return NULL;
		}
		/* this imitates list_last_entry(). */
		return list_entry(active->queue[idx].prev,
						  resch_task_t,
						  active_entry);
	}
	return list_entry(rt->active_entry.prev,
					  resch_task_t,
					  active_entry);
}
EXPORT_SYMBOL(active_prev_prio_task);

/**
 * return the first task which has the given priority.
 * return NULL if the CPU has no tasks with the priority.
 * the active queue must be locked.
 */
resch_task_t* active_prio_task(int cpu, int prio)
{
	int idx = prio_index(prio);
	struct prio_array *active = &lo[cpu].active;
	struct list_head *queue = &active->queue[idx];

	if (list_empty(queue)) {
		return NULL;
	}
	return list_first_entry(queue, resch_task_t, active_entry);
}
EXPORT_SYMBOL(active_prio_task);

/**
 * return the task running on the given CPU, which has the num-th priority.
 * return NULL if the CPU has no ready tasks managed by Resch.
 * the active queue must be locked.
 */
resch_task_t* active_number_task(int cpu, int num)
{
	int i;
	int idx;
	resch_task_t *rt;
	struct prio_array *active = &lo[cpu].active;

	i = idx = 0;
	while (i < num) {
		/* find next set bit with offset of idx. */
		if ((idx = resch_fns(active->bitmap, idx, RESCH_PRIO_LONG)) < 0) {
			return NULL;
		}
		/* should not empty! */
		list_for_each_entry(rt, &active->queue[idx], active_entry) {
			if (++i == num) {
				return rt;
			}
		}
		/* set the index for the next search.
		   this also prevents an infinite loop. */
		idx++;
	}
	/* should not reach here... */
	RESCH_DPRINT("active queue may be broken on CPU#%d .\n", cpu);
	return NULL;
}
EXPORT_SYMBOL(active_number_task);

/**
 * return the highest-priority task submitted to RESCH.
 * the global list must be locked.
 */
resch_task_t* global_highest_prio_task(void)
{
	if (list_empty(&task_list.head)) {
		return NULL;
	}
	return list_first_entry(&task_list.head,
							resch_task_t,
							global_entry);
}
EXPORT_SYMBOL(global_highest_prio_task);

/**
 * return the task positioned at the next of @rt in the global list.
 * the global list must be locked.
 */
resch_task_t* global_next_prio_task(resch_task_t *rt)
{
	if (rt->global_entry.next == &(task_list.head)) {
		return NULL;
	}
	return list_entry(rt->global_entry.next,
					  resch_task_t,
					  global_entry);
}
EXPORT_SYMBOL(global_next_prio_task);

/**
 * return the task positioned at the previous of @rt in the global list.
 * the global list must be locked.
 */
resch_task_t* global_prev_prio_task(resch_task_t *rt)
{
	if (rt->global_entry.prev == &(task_list.head)) {
		return NULL;
	}
	return list_entry(rt->global_entry.prev,
					  resch_task_t,
					  global_entry);
}
EXPORT_SYMBOL(global_prev_prio_task);

/**
 * return the first task that has the given priority or less.
 * the global list must be locked.
 */
resch_task_t* global_prio_task(int prio)
{
	resch_task_t *rt;
	list_for_each_entry(rt, &task_list.head, global_entry) {
		if (rt->prio <= prio) {
			return rt;
		}
	}
	return NULL;
}
EXPORT_SYMBOL(global_prio_task);

/**
 * return the task positioned at the num-th in the global list.
 * the global list must be locked.
 */
resch_task_t* global_number_task(int num)
{
	int i = 0;
	resch_task_t *rt;
	list_for_each_entry(rt, &task_list.head, global_entry) {
		if (++i == num) {
			return rt;
		}
	}
	return NULL;
}
EXPORT_SYMBOL(global_number_task);

/**
 * lock the active queue on the given CPU.
 */
void active_queue_lock(int cpu, unsigned long *flags)
{
	spin_lock_irqsave(&lo[cpu].active.lock, *flags);
}
EXPORT_SYMBOL(active_queue_lock);

/**
 * unlock the active queue on the given CPU.
 */
void active_queue_unlock(int cpu, unsigned long *flags)
{
	spin_unlock_irqrestore(&lo[cpu].active.lock, *flags);
}
EXPORT_SYMBOL(active_queue_unlock);

/**
 * lock the two active queues on the given CPUs at the same time.
 * this function may be useful on task migrations.
 */
void active_queue_double_lock(int cpu1, int cpu2, unsigned long *flags)
{
	spin_lock_irqsave(&lo[cpu1].active.lock, *flags);
	spin_lock(&lo[cpu2].active.lock);
}
EXPORT_SYMBOL(active_queue_double_lock);

/**
 * unlock the two active queues on the given CPUs at the same time.
 * this function may be useful on task migrations.
 */
void active_queue_double_unlock(int cpu1, int cpu2, unsigned long *flags)
{
	spin_unlock(&lo[cpu2].active.lock);
	spin_unlock_irqrestore(&lo[cpu1].active.lock, *flags);
}
EXPORT_SYMBOL(active_queue_double_unlock);

/**
 * down the mutex to lock the global list.
 */
void global_list_down(void)
{
	down(&task_list.sem);
}
EXPORT_SYMBOL(global_list_down);

/**
 * up the mutex to unlock the global list.
 */
void global_list_up(void)
{
	up(&task_list.sem);
}
EXPORT_SYMBOL(global_list_up);

/**
 * migrate @rt to the given CPU.
 */
void migrate_task(resch_task_t *rt, int cpu_dst)
{
	/* call the class function. */
	rt->class->migrate_task(rt, cpu_dst);
}
EXPORT_SYMBOL(migrate_task);

/**
 * return the scheduling overhead in the given interval by microseconds.
 * since we dont know the single scheduler tick cost, we pessimistically
 * assume that context switches may happen at every scheduler tick.
 * hence, the total number of context switches assumed is the sum of
 * the number of job completions and the number of scheduler ticks.
 * to reduce pessimism, we also assume that the single scheduler tick
 * cost is as half as the single context switch cost, given that a kernel
 * timer interrupt does not store and load all registers but does less
 * than half of registers.
 * we believe this assumption is still pessimistic.
 */
unsigned long sched_overhead_cpu(int cpu, unsigned long interval)
{
	resch_task_t *rt;
	unsigned long nr_jobs;
	unsigned long overhead = 0;
	list_for_each_entry(rt, &task_list.head, global_entry) {
		if (task_is_on_cpu(rt, cpu)) {
			nr_jobs = div_round_up(interval,
									jiffies_to_usecs(rt->period));
			overhead += nr_jobs * (switch_cost + release_cost);
			if (rt->migratory) {
				overhead += nr_jobs * migration_cost;
			}
		}
	}
	return overhead + usecs_to_jiffies(interval) * switch_cost / 2;
}
EXPORT_SYMBOL(sched_overhead_cpu);

unsigned long sched_overhead_task(resch_task_t *rt, unsigned long interval)
{
	unsigned long nr_jobs;
	unsigned long overhead;
	nr_jobs = div_round_up(interval, jiffies_to_usecs(rt->period));
	overhead =  nr_jobs * (switch_cost + release_cost);
	if (rt->migratory) {
		overhead += nr_jobs * migration_cost;
	}
	return overhead;
}
EXPORT_SYMBOL(sched_overhead_task);

/**
 * a kernel thread function to alternatively call sched_setscheduler() to
 * change the scheduling policy and the priority of real-time tasks.
 */
static void set_scheduler_thread(void *__data)
{
	int cpu = (long) __data;
	struct resch_task_struct *rt;
	struct sched_thread_struct *thread = &lo[cpu].sched_thread;

	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		spin_lock_irq(&thread->lock);
		if (list_empty(&thread->list)) {
			spin_unlock_irq(&thread->lock);
			schedule();
			set_current_state(TASK_INTERRUPTIBLE);
			continue;
		}

		/* get a task in the list by fifo. */
		rt = list_first_entry(&thread->list, resch_task_t, prio_entry);
		/* remove this task from the list. */
		list_del_init(&rt->prio_entry);
		spin_unlock(&thread->lock);

		/* alternatively set the scheduler for the task. */
		__set_scheduler(rt, rt->setsched.prio);
		wake_up_process(rt->setsched.caller);
		local_irq_enable();
	}
}

/**
 * initialize the RESCH task descriptor members.
 */
static void resch_task_init(resch_task_t *rt, int rid)
{
	int cpu;

	rt->rid = rid;
	rt->task = current;
	/* limit available CPUs by NR_RT_CPUS. */
#ifdef USE_VIVID_OR_OLDER
	cpus_clear(rt->cpumask);
#else
	cpumask_clear(&rt->cpumask);
#endif
	for (cpu = 0; cpu < NR_RT_CPUS; cpu++) {
#ifdef USE_VIVID_OR_OLDER
		cpu_set(cpu, rt->cpumask); 
#else
		cpumask_set_cpu(cpu,&rt->cpumask);
#endif
	}
	cpumask_and(&rt->task->cpus_allowed,
				&rt->task->cpus_allowed, &rt->cpumask);

	/* migrate the task to some CPU in range, if necessary. */
	if (smp_processor_id() >= NR_RT_CPUS) {
		__migrate_task(rt, jiffies % NR_RT_CPUS);
	}

	rt->prio = RESCH_PRIO_MIN;
	rt->cpu_id = RESCH_CPU_UNDEFINED;
	rt->migratory = true;
	rt->missed = false;
	rt->wcet_fixed = false;
	rt->wcet = 0;
	rt->runtime = 0;
	rt->period = 0;
	rt->deadline = 0; /* relative deadline. */
	rt->deadline_time = 0; /* absolute deadline. */
	rt->release_time = 0;
#ifdef RESCH_PREEMPT_TRACE
	rt->preempter = NULL;
	rt->preemptee = NULL;
#endif
	rt->class = &fp_sched_class;
	rt->policy = RESCH_SCHED_FP; /* fair scheduling by default. */
	rt->accounting = false;
	rt->xcpu = false;
	rt->reserve_time = 0;
	rt->server = NULL;
	INIT_LIST_HEAD(&rt->global_entry);
	INIT_LIST_HEAD(&rt->active_entry);
	INIT_LIST_HEAD(&rt->component_entry);
}

/**
 * if there are dead tasks, clear corresponding PID bitmap.
 */
static void clear_dead_tasks(void)
{
	int i;
	struct task_struct *p;

	for (i = 0; i < NR_RT_TASKS; i++) {
		p = resch_task[i].task;
		if (p && p->state == TASK_STOPPED) {
			/* clear the bit for the corresponding RESCH ID. */
			__clear_bit(i, pid_map.bitmap);
		}
	}
}

/**
 * API: set a ROS node index to RESCH.
 */
node_t* root_node;
int api_set_node(int rid, unsigned long node_index)
{
    printk(KERN_INFO
           "RESCH: get ROS node index:%d.\n", node_index);

    switch (node_index) {
    case 0: /* rosout */
    {
        /*
        root_node = make_node(16);
        node_t* node15 = make_node(15);
        node_t* node14 = make_node(14);
        node_t* node13 = make_node(13);
        node_t* node12 = make_node(12);
        node_t* node11 = make_node(11);
        node_t* node21 = make_node(21);
        node_t* node22 = make_node(22);
        node_t* node23 = make_node(23);
        node_t* node31 = make_node(31);
        node_t* node32 = make_node(32);
        node_t* node41 = make_node(41);
        insert_child_node(root_node, node15);
        insert_child_node(root_node, node41);
        insert_child_node(node15, node14);
        insert_child_node(node15, node32);
        insert_child_node(node14, node13);
        insert_child_node(node13, node12);
        insert_child_node(node13, node23);
        insert_child_node(node12, node11);
        insert_child_node(node23, node22);
        insert_child_node(node22, node21);
        insert_child_node(node32, node31);
        insert_child_node(node31, node21);
        insert_child_node(node41, node21);
        printk(KERN_INFO
               "Show tree.\n");
        show_tree_dfs(root_node);
        break;
        */
    }
    case 1:
    {
        /*
        node_t* result = search_node(root_node, node_index);
        if(result != NULL) {
            result->is_exist = 1;
            printk(KERN_INFO
                   "node[%d] is found.\n", node_index);
        } else {
            printk(KERN_INFO
                   "node[%d] not found.\n", node_index);
        }
        search_leaf_node(root_node);
        free_tree(root_node);
        break;
        */
    }
    case 16:
    case 15:
    case 14:
    case 13:
    case 12:
    case 11:
    case 23:
    case 22:
    case 21:
    case 31:
    {
        break;
    }
    case 41:
    {

        node_t* result = search_node(root_node, node_index);
        if(result != NULL) {
            result->is_exist = 1;
            printk(KERN_INFO
                   "node[%d] is found.\n", result->index);
//            api_set_scheduler(rid, RESCH_SCHED_FP);
//            api_set_priority(rid, 98);
        } else {
            printk(KERN_INFO
                   "not found.\n");
        }
        break;
    }

    default:
            break;
    }

    return rid;
}

/**
 * API: start callback function on the ROS node.
 */
int api_start_callback(int rid, unsigned long node_index)
{
    printk(KERN_INFO
           "RESCH: start callback (node index:%d).\n", node_index);

    return rid;
}

/**
 * API: end callback function on the ROS node.
 */
int api_end_callback(int rid, unsigned long node_index)
{
    printk(KERN_INFO
           "RESCH: start callback (node index:%d).\n", node_index);

    return rid;
}

/**
 * API: attach the current task to RESCH.
 * note that the current task still remains as a fair task.
 * it is turned into a real-time task when api_set_scheduler() or
 * api_set_priority() is called.
 */
int api_init(void)
{
	int rid;

	/* find an available RESCH ID. */
	spin_lock_irq(&pid_map.lock);
	if ((rid = resch_ffz(pid_map.bitmap, PID_MAP_LONG)) < 0) {
		/* error print. */
		printk(KERN_WARNING
			   "RESCH: failed to attach the module.\n");
		/* clear all dead PIDs. */
		clear_dead_tasks();
		spin_unlock_irq(&pid_map.lock);
		goto out;
	}
	/* set the bit for the corresponding RESCH ID. */
	__set_bit(rid, pid_map.bitmap);
	spin_unlock_irq(&pid_map.lock);

	/* init the resch task discriptor. */
	resch_task_init(&resch_task[rid], rid);

 out:
	return rid;
}

/**
 * API: detach the current task from RESCH.
 */
int api_exit(int rid)
{
	int res = RES_SUCCESS;
	resch_task_t *rt = resch_task_ptr(rid);

	RESCH_DPRINT("task#%d is exiting...\n", rid);

	/* make sure to complete the job. */
	if (task_is_active(rt)) {
		job_complete(rt);
	}

	/* make sure to stop reservation. */
	if (task_has_reserve(rt)) {
		reserve_stop(rt);
	}

	/* make sure to remove the task from the task list. */
	if (task_is_managed(rt)) {
		global_list_remove(rt);
	}

	/* call the plugin function. */
	sched_plugins.task_exit(rt);

	/* clear the bit for the corresponding RESCH ID. */
	spin_lock_irq(&pid_map.lock);
	__clear_bit(rid, pid_map.bitmap);
	spin_unlock_irq(&pid_map.lock);

	/* put back the scheduling policy and priority. */
	if (rt->policy != RESCH_SCHED_FAIR) {
		set_scheduler(rt, RESCH_SCHED_FAIR, 0);
	}

	return res;
}

/**
 * API: run the current task.
 * the first job will be released when @timeout (ms) elapses.
 */
int api_run(int rid, unsigned long timeout)
{
	resch_task_t *rt = resch_task_ptr(rid);

	/* insert the task into the global list. */
	global_list_insert(rt);

	/* call a plugin function. */
	sched_plugins.task_run(rt);
	if (rt->policy == RESCH_SCHED_FAIR) {
		global_list_remove(rt);
	}

	job_release(rt, jiffies + msecs_to_jiffies(timeout));
	job_start(rt);

	return RES_SUCCESS;
}

/**
 * API: let the current task wait for the next period.
 */
int api_wait_for_period(int rid)
{
	int res = RES_SUCCESS;
	resch_task_t *rt = resch_task_ptr(rid);

	if (deadline_miss(rt)) {
		res = RES_MISS;
	}

	job_complete(rt);
	job_release(rt, rt->release_time + rt->period);
	job_start(rt);

	return res;
}

int api_wait_for_interval(int rid, unsigned long period)
{
	int res = RES_SUCCESS;
	resch_task_t *rt = resch_task_ptr(rid);

	if (deadline_miss(rt)) {
		res = RES_MISS;
	}

	job_complete(rt);
	job_release(rt, jiffies + period);
	job_start(rt);

	return res;
}

/**
 * API: set (change) the period of the current task.
 */
int api_set_period(int rid, unsigned long period)
{
	resch_task_t *rt = resch_task_ptr(rid);
	rt->period = period;
	/* if the deadline has not yet been set, assign the period to it. */
	if (rt->deadline == 0) {
		rt->deadline = rt->period;
	}
	return RES_SUCCESS;
}

/**
 * API: set (change) the relative deadline of the current task.
 */
int api_set_deadline(int rid, unsigned long deadline)
{
	resch_task_t *rt = resch_task_ptr(rid);
	rt->deadline = deadline;
	return RES_SUCCESS;
}

/**
 * API: set (change) the worst-case executiont time of the current task.
 * @wcet is given by microseconds.
 */
int api_set_wcet(int rid, unsigned long wcet)
{
	resch_task_t *rt = resch_task_ptr(rid);
	rt->wcet = wcet;
	if (rt->runtime == 0) {
		rt->runtime = wcet;
	}
	rt->wcet_fixed = true;
	return RES_SUCCESS;
}

/**
 * API: set (change) the average execution time of the current task.
 * @runtime is given by microseconds.
 */
int api_set_runtime(int rid, unsigned long runtime)
{
	resch_task_t *rt = resch_task_ptr(rid);
	rt->runtime = runtime;
	if (rt->wcet == 0) {
		rt->wcet = runtime;
	}
	return RES_SUCCESS;
}

/**
 * API: set (change) the priority of the current task.
 */
int api_set_priority(int rid, unsigned long prio)
{
	resch_task_t *rt = resch_task_ptr(rid);

	/* dont allow to set the priority for the EDF and FAIR schedulers. */
	if (rt->policy == RESCH_SCHED_EDF || rt->policy == RESCH_SCHED_FAIR) {
		return RES_FAULT;
	}

	/* set only the priority by set_scheduler(). */
	if (!set_scheduler(rt, rt->policy, prio)) {
		return RES_FAULT;
	}
	return RES_SUCCESS;
}

/**
 * API: set the scheduling policy for the current task.
 */
int api_set_scheduler(int rid, unsigned long policy)
{
	resch_task_t *rt = resch_task_ptr(rid);

	if (policy != rt->policy) {
		/* set only the scheduling policy by set_scheduler(). */
		if (!set_scheduler(rt, policy, rt->prio)) {
			return RES_FAULT;
		}
	}

	return RES_SUCCESS;
}

/**
 * API: set affinity for the current task.
 */
//int api_set_affinity(int rid, size_t cpusetsize, const cpu_set_t *mask)
//{
//    resch_task_t *rt = resch_task_ptr(rid);

//    /* dont allow to set the priority for the EDF and FAIR schedulers. */
//    if (rt->policy == RESCH_SCHED_EDF || rt->policy == RESCH_SCHED_FAIR) {
//        return RES_FAULT;
//    }

//    /* set only the priority by set_scheduler(). */
//    if (!set_affinity(rt, cpusetsize, mask)) {
//        return RES_FAULT;
//    }
//    return RES_SUCCESS;
//}

/**
 * API: schedule the current task in background.
 */
int api_background(int rid)
{
	return api_set_priority(rid, RESCH_PRIO_BACKGROUND);
}

void sched_init(void)
{
	int i;
	int cpu;
	struct sched_param sp = { .sched_priority = RESCH_PRIO_KTHREAD };

	uninstall_scheduler();

	sema_init(&task_list.sem, 1);
	INIT_LIST_HEAD(&task_list.head);

	for (i = 0; i < PID_MAP_LONG; i++) {
		pid_map.bitmap[i] = 0;
	}

	for (cpu = 0; cpu < NR_RT_CPUS + 1; cpu++) {
		/* active task queue. */
		lo[cpu].active.nr_tasks = 0;
		for (i = 0; i < RESCH_PRIO_LONG; i++) {
			lo[cpu].active.bitmap[i] = 0;
		}
		for (i = 0; i < RESCH_PRIO_MAX; i++) {
			INIT_LIST_HEAD(lo[cpu].active.queue + i);
		}
#ifdef RESCH_PREEMPT_TRACE
		/* CPU tick. */
		lo[cpu].last_tick = jiffies;
		/* current task. */
		lo[cpu].current_task = NULL;
#endif
	}
	for (cpu = 0; cpu < NR_RT_CPUS; cpu++) {
		/* the setscheduler thread. */
		INIT_LIST_HEAD(&lo[cpu].sched_thread.list);
		lo[cpu].sched_thread.task =
			kthread_create((void*)set_scheduler_thread,
						   (void*)(long)cpu,
						   "resch-kthread");
		if (lo[cpu].sched_thread.task != ERR_PTR(-ENOMEM)) {
			kthread_bind(lo[cpu].sched_thread.task, cpu);
			sched_setscheduler(lo[cpu].sched_thread.task,
							   SCHED_FIFO,
							   &sp);
			wake_up_process(lo[cpu].sched_thread.task);
		}
		else {
			printk(KERN_INFO "RESCH: failed to create a kernel thread\n");
			lo[cpu].sched_thread.task = NULL;
		}
	}
}

void sched_exit(void)
{
	int cpu;

	for (cpu = 0; cpu < NR_RT_CPUS; cpu++) {
		if (lo[cpu].sched_thread.task) {
			kthread_stop(lo[cpu].sched_thread.task);
		}
	}
}
