/*
 * edf-us.c:		Copyright (C) Shinpei Kato
 *
 * Global EDF with Utilization Separation (EDF-US) scheduler.
 *
 * It is designed based on the algoritm presented in:
 * - Deadline-based Scheduling of Periodic Task Systems on Multiprocessors
 *   (Srinivasan et al., Information Processing Letters, 2002).
 *
 * Currently, the EDF-US scheduler does not support synchornization and 
 * self-suspension. We do not guarantee performance when they are used.
 * We will however soon support these mechanisms.
 */

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <resch-core.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("EDF-US Scheduler");
MODULE_AUTHOR("Shinpei Kato");

#define MODULE_NAME	"edf-us"
#define EDF_US_CPU_MAIN	0
#define EDF_US_PRIO_HIGH	(RESCH_PRIO_KTHREAD - 1)

/**
 * Task descriptor specific for EDF-US.
 */
typedef struct edf_us_task_struct {
	resch_task_t *rt;
	struct list_head release_entry;
	struct list_head run_entry;
	struct list_head wait_entry;
} edf_us_task_t;
edf_us_task_t edf_us_task[NR_RT_TASKS];

struct edf_us_thread_struct {
	struct task_struct *task;
	struct list_head list;
	spinlock_t lock;
} edf_us_thread[NR_RT_CPUS];

int cpu_run[NR_RT_CPUS] = {[0 ... NR_RT_CPUS-1] = false};

struct list_head run_list;
struct list_head wait_list;
struct semaphore edf_us_sem;
spinlock_t edf_us_lock;

static void	run_list_insert(edf_us_task_t *gt)
{
	edf_us_task_t *p;

	if (!list_empty(&gt->run_entry)) {
		return;
	}
	
	if (list_empty(&run_list)) {
		list_add_tail(&gt->run_entry, &run_list);
	}
	else {
		list_for_each_entry(p, &run_list, run_entry) {
			if (gt->rt->deadline_time < p->rt->deadline_time) {
				/* insert @gt before @p. */
				list_add_tail(&gt->run_entry, &p->run_entry);
				return;
			}
		}
		list_add_tail(&gt->run_entry, &run_list);
	}
}

static void run_list_remove(edf_us_task_t *gt)
{
	list_del_init(&gt->run_entry);
}

static void	wait_list_insert(edf_us_task_t *gt)
{
	edf_us_task_t *p;

	if (!list_empty(&gt->wait_entry)) {
		return;
	}
	
	if (list_empty(&wait_list)) {
		list_add_tail(&gt->wait_entry, &wait_list);
		return;
	}
	else {
		list_for_each_entry(p, &wait_list, wait_entry) {
			if (gt->rt->deadline_time < p->rt->deadline_time) {
				/* insert @gt before @p. */
				list_add_tail(&gt->wait_entry, &p->wait_entry);
				return;
			}
		}
		list_add_tail(&gt->wait_entry, &wait_list);
	}
}

static void wait_list_remove(edf_us_task_t *gt)
{
	list_del_init(&gt->wait_entry);
}

static edf_us_task_t* get_lowest_prio_task(void)
{
	if (list_empty(&run_list)) {
		return NULL;
	}
	return list_entry(run_list.prev, edf_us_task_t, run_entry);
}

static edf_us_task_t* get_next_task(void)
{
	if (list_empty(&wait_list)) {
		return NULL;
	}
	return list_first_entry(&wait_list, edf_us_task_t, wait_entry);
}

static void set_high_prio(edf_us_task_t *et)
{
	set_scheduler(et->rt, RESCH_SCHED_FP, EDF_US_PRIO_HIGH);
}

static void set_edf_prio(edf_us_task_t *et)
{
#ifdef SCHED_DEADLINE
	et->rt->task->dl.deadline = 0;
#endif
	set_scheduler(et->rt, RESCH_SCHED_EDF, RESCH_PRIO_EDF);
}

/**
 * internal function to start a job of the given task.
 */
static void __job_start(edf_us_task_t *gt)
{
	int cpu_dst;
	edf_us_task_t *p = NULL;

	spin_lock_irq(&edf_us_lock);
	if (!cpu_run[gt->rt->cpu_id]) {
		cpu_dst = gt->rt->cpu_id;
		goto run;
	}
	else {
		for (cpu_dst = 0; cpu_dst < NR_RT_CPUS; cpu_dst++) {
			if (!cpu_run[cpu_dst]) {
				goto run;
			}
		}
		p = get_lowest_prio_task();
		if (p && p->rt->deadline_time > gt->rt->deadline_time) {
			run_list_remove(p);
			wait_list_insert(p);
			cpu_dst = p->rt->cpu_id;
			goto run;
		}
	}
	wait_list_insert(gt);
	spin_unlock_irq(&edf_us_lock);

	/* put the original priority. */
	set_edf_prio(gt);

	return;

 run:
	cpu_run[cpu_dst] = true;
	run_list_insert(gt);
	spin_unlock_irq(&edf_us_lock);

	if (gt->rt->cpu_id != cpu_dst) {
		down(&edf_us_sem);
		migrate_task(gt->rt, cpu_dst);
		up(&edf_us_sem);
	}

	/* put the original priority. */
	set_edf_prio(gt);
}

/**
 * plugin function to submit the given task.
 * migrate the given task to the main CPU before execution.
 */
static void task_run(resch_task_t *rt)
{
	int u = usecs_to_jiffies(rt->wcet * 100) / rt->period;
	edf_us_task_t *p = &edf_us_task[rt->rid];
	if (u > NR_RT_CPUS * 100 / (2 * NR_RT_CPUS - 1)) {
		set_scheduler(rt, RESCH_SCHED_FP, EDF_US_PRIO_HIGH);
	}

	p->rt = rt;
	INIT_LIST_HEAD(&p->run_entry);
	INIT_LIST_HEAD(&p->wait_entry);
	set_high_prio(p);
	migrate_task(rt, EDF_US_CPU_MAIN);
}

/**
 * plugin function to start a job of the given task.
 */
static void job_start(resch_task_t *rt)
{
	edf_us_task_t *p = &edf_us_task[rt->rid];
	__job_start(p);
}

/**
 * plugin function to complete the current job of the given task.
 * this function migrates such a job that has the highest priority 
 * other than the current job to this CPU.
 */
static void job_complete(resch_task_t *rt)
{
	edf_us_task_t *p = &edf_us_task[rt->rid];
	edf_us_task_t *next;

	/* put the highest priority so that it can react to the next period. */
	set_high_prio(p);

	spin_lock_irq(&edf_us_lock);
	if (!list_empty(&p->wait_entry)) {
		wait_list_remove(p);
	} 
	else {
		run_list_remove(p);
	}
	next = get_next_task();
	if (next) {
		wait_list_remove(next);
		run_list_insert(next);
	}
	else {
		cpu_run[rt->cpu_id] = false;
	}
	spin_unlock_irq(&edf_us_lock);

	/* the next task will never preempt the current task. */
	if (next) {
		if (next->rt->cpu_id != rt->cpu_id) {
			down(&edf_us_sem);
			migrate_task(next->rt, rt->cpu_id);
			up(&edf_us_sem);
		}
	}
}

static int __init edf_us_init(void)
{
	printk(KERN_INFO "EDF-US: HELLO!\n");
	install_scheduler(task_run, NULL, job_start, job_complete);
	INIT_LIST_HEAD(&run_list);
	INIT_LIST_HEAD(&wait_list);
	sema_init(&edf_us_sem, 1);
	
	return 0;
}

static void __exit edf_us_exit(void)
{
	printk(KERN_INFO "EDF-US: GOODBYE!\n");
	uninstall_scheduler();
}

module_init(edf_us_init);
module_exit(edf_us_exit);
