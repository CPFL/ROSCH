/*
 * g-fp.c:		Copyright (C) Shinpei Kato
 *
 * Global fixed-priority (G-FP) scheduler.
 */

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <resch-core.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("G-FP Scheduler");
MODULE_AUTHOR("Shinpei Kato");

#define MODULE_NAME		"g-fp"
#define GFP_CPU_MAIN	0
#define GFP_PRIO_HIGH	(RESCH_PRIO_KTHREAD - 1)

/**
 * Task descriptor specific for G-FP.
 */
typedef struct gfp_task_struct {
	resch_task_t *rt;
	struct list_head release_entry;
	struct list_head run_entry;
	struct list_head wait_entry;
	int prio;
} gfp_task_t;
gfp_task_t gfp_task[NR_RT_TASKS];

struct gfp_thread_struct {
	struct task_struct *task;
	struct list_head list;
	spinlock_t lock;
} gfp_thread[NR_RT_CPUS];

int cpu_run[NR_RT_CPUS] = {[0 ... NR_RT_CPUS-1] = false};

struct list_head run_list;
struct list_head wait_list;
struct semaphore gfp_sem;
spinlock_t gfp_lock;

static void	run_list_insert(gfp_task_t *gt)
{
	gfp_task_t *p;

	if (!list_empty(&gt->run_entry)) {
		return;
	}
	
	if (list_empty(&run_list)) {
		list_add_tail(&gt->run_entry, &run_list);
	}
	else {
		list_for_each_entry(p, &run_list, run_entry) {
			if (gt->rt->prio > p->rt->prio) {
				/* insert @gt before @p. */
				list_add_tail(&gt->run_entry, &p->run_entry);
				return;
			}
		}
		list_add_tail(&gt->run_entry, &run_list);
	}
}

static void run_list_remove(gfp_task_t *gt)
{
	list_del_init(&gt->run_entry);
}

static void	wait_list_insert(gfp_task_t *gt)
{
	gfp_task_t *p;

	if (!list_empty(&gt->wait_entry)) {
		return;
	}
	
	if (list_empty(&wait_list)) {
		list_add_tail(&gt->wait_entry, &wait_list);
		return;
	}
	else {
		list_for_each_entry(p, &wait_list, wait_entry) {
			if (gt->rt->prio > p->rt->prio) {
				/* insert @gt before @p. */
				list_add_tail(&gt->wait_entry, &p->wait_entry);
				return;
			}
		}
		list_add_tail(&gt->wait_entry, &wait_list);
	}
}

static void wait_list_remove(gfp_task_t *gt)
{
	list_del_init(&gt->wait_entry);
}

static gfp_task_t* get_lowest_prio_task(void)
{
	if (list_empty(&run_list)) {
		return NULL;
	}
	return list_entry(run_list.prev, gfp_task_t, run_entry);
}

static gfp_task_t* get_next_task(void)
{
	if (list_empty(&wait_list)) {
		return NULL;
	}
	return list_first_entry(&wait_list, gfp_task_t, wait_entry);
}

static void set_high_prio(gfp_task_t *ft)
{
	set_scheduler(ft->rt, RESCH_SCHED_FP, GFP_PRIO_HIGH);
}

static void set_fp_prio(gfp_task_t *ft)
{
	set_scheduler(ft->rt, RESCH_SCHED_FP, ft->prio);
}

/**
 * internal function to start a new job of the given task.
 */
static void __job_start(gfp_task_t *gt)
{
	int cpu_dst;
	gfp_task_t *p = NULL;

	spin_lock_irq(&gfp_lock);
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
		if (p && p->rt->prio < gt->rt->prio) {
			run_list_remove(p);
			wait_list_insert(p);
			cpu_dst = p->rt->cpu_id;
			goto run;
		}
	}
	wait_list_insert(gt);
	spin_unlock_irq(&gfp_lock);

	/* put the original priority. */
	set_fp_prio(gt);

	return;

 run:
	cpu_run[cpu_dst] = true;
	run_list_insert(gt);
	spin_unlock_irq(&gfp_lock);

	if (gt->rt->cpu_id != cpu_dst) {
		down(&gfp_sem);
		migrate_task(gt->rt, cpu_dst);
		up(&gfp_sem);
	}

	/* put the original priority. */
	set_fp_prio(gt);
}

/**
 * plugin function to submit the given task.
 * migrate the given task to the main CPU before execution.
 */
void task_run(resch_task_t *rt)
{
	gfp_task_t *p = &gfp_task[rt->rid];
	p->rt = rt;
	INIT_LIST_HEAD(&p->run_entry);
	INIT_LIST_HEAD(&p->wait_entry);
	p->prio = rt->prio;
	set_high_prio(p);
	migrate_task(rt, GFP_CPU_MAIN);
}

/**
 * plugin function to start a new job of the given task.
 */
void job_start(resch_task_t *rt)
{
	gfp_task_t *p = &gfp_task[rt->rid];
	__job_start(p);
}

/**
 * plugin function to complete the current job of the given task.
 * this function migrates such a task that has the highest priority 
 * other than the current tasks to this CPU.
 */
void job_complete(resch_task_t *rt)
{
	gfp_task_t *p = &gfp_task[rt->rid];
	gfp_task_t *next;

	set_high_prio(p);

	spin_lock_irq(&gfp_lock);
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
	spin_unlock_irq(&gfp_lock);

	/* the next task will never preempt the current task. */
	if (next) {
		if (next->rt->cpu_id != rt->cpu_id) {
			down(&gfp_sem);
			migrate_task(next->rt, rt->cpu_id);
			up(&gfp_sem);
		}
	}
}

static int __init gfp_init(void)
{
	printk(KERN_INFO "G-FP: HELLO!\n");
	install_scheduler(task_run, NULL, job_start, job_complete);
	INIT_LIST_HEAD(&run_list);
	INIT_LIST_HEAD(&wait_list);
	sema_init(&gfp_sem, 1);
	
	return 0;
}

static void __exit gfp_exit(void)
{
	printk(KERN_INFO "G-FP: GOODBYE!\n");
	uninstall_scheduler();
}

module_init(gfp_init);
module_exit(gfp_exit);
