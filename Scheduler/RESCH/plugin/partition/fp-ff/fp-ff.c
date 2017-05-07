/*
 * fp-ff.c:			Copyright (C) Shinpei Kato
 *
 * Fixed-Priority with First-Fit (FP-FF) partitioning scheduler.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <resch-core.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("FP-FF Scheduler");
MODULE_AUTHOR("Shinpei Kato");

#define MODULE_NAME	"fp-ff"
#ifdef RESCH_HARD_RT
#define C wcet
#else /* RESCH_SOFT_RT */
#define C runtime
#endif

/**
 * compute the worst-case response time of @rt on the given CPU. 
 * it is undefined if the worst-case response time exceeds the range 
 * of size unsigned long...
 * the global list must be locked.
 */
static unsigned long response_time_analysis(resch_task_t *rt, int cpu) 
{
	int F;
	unsigned long ret;
	unsigned long Pk, Ck;
	unsigned long L;
	unsigned long overhead;
	resch_task_t *hp;

	ret = 0;
	overhead = 0;
	L = jiffies_to_usecs(rt->deadline);
	hp = global_highest_prio_task();
	while (hp && hp->prio >= rt->prio) {
		if (task_is_on_cpu(hp, cpu) && hp != rt) {
			Pk = jiffies_to_usecs(hp->period);
			Ck = hp->C;
			F = L / Pk;
			if (L >= Pk * F + Ck) {
				ret += Ck * (F + 1);
			}
			else {
				ret += L - F * (Pk - Ck);
			}
		}
		hp = global_next_prio_task(hp);
	}

	return ret + rt->C + 
		sched_overhead_cpu(cpu, L) + sched_overhead_task(rt, L);
}

/**
 * try to assign the given task to a particular CPU.
 */
static int partition(resch_task_t *rt)
{
	int cpu;
	int success;
	int F;
	unsigned long Ri, Di, Pi, Ci;
	unsigned long Rk, Dk;
	resch_task_t *lp;

	/* deadline, period, and execution time. */
	Di = jiffies_to_usecs(rt->deadline);
	Pi = jiffies_to_usecs(rt->period);
	Ci = rt->C;

	for (cpu = 0; cpu < NR_RT_CPUS; cpu++) {
		/* check the default CPU mask. */
		if (!cpu_isset(cpu, rt->task->cpus_allowed)) {
			continue;
		}

		/* first, get the response time of the task on this cpu. */
		Ri = response_time_analysis(rt, cpu);
		if (Ri > Di) {
			continue;
		}

		/* skip all higher-priority tasks, and get the first task whose
		   priority is less than or equal to @rt. */
		lp = global_prio_task(rt->prio);

		/* for the tasks with priorities lower than or equal to @rt. */
		success = true;
		while (lp) {
			if (!task_is_on_cpu(lp, cpu)) {
				lp = global_next_prio_task(lp);
				continue;
			}
			Rk = response_time_analysis(lp, cpu);
			Dk = jiffies_to_usecs(lp->deadline);
			lp = global_next_prio_task(lp);

			/* take also into account @rt. */
			F = Dk / Pi;
			if (Dk >= Pi * F + Ci) {
				Rk += Ci * (F + 1);
			}
			else {
				Rk += Dk - F * (Pi - Ci);
			}

			/* get unschedulable? */
			if (Rk > Dk) {
				success = false;
				break;
			}
		}

		if (success) {
			return cpu;
		}
	}
	
	return RESCH_CPU_UNDEFINED;
}

/**
 * assign the given task to a particular CPU.
 * if no CPUs can accept the task, it is just assigned the lowest priority.
 * note that the task has not yet been inserted into the global list.
 */
void task_run(resch_task_t *rt)
{
	int cpu_dst;

	/* aperiodic tasks are just skipped. */
	if (!rt->period) {
		return ;
	}

	/* this is going to be a big lock! */
	global_list_down();

	/* try partitioning. */
	cpu_dst = partition(rt);
	rt->cpu_id = cpu_dst;

	global_list_up();

	/* if partitioning succeeded, migrate @p to the cpu. 
	   otherwise, do the default fair scheduling. */
	if (cpu_dst != RESCH_CPU_UNDEFINED) {
		if (rt->policy != RESCH_SCHED_FP) {
			set_scheduler(rt, RESCH_SCHED_FP, rt->prio);
		}
		printk(KERN_INFO "FP-PM: task#%d is assigned to CPU#%d.\n", 
			   rt->rid, cpu_dst);
		rt->migratory = false;
		migrate_task(rt, cpu_dst);
	}
	else {
		printk(KERN_INFO "FP-PM: task#%d is not schedulable.\n", rt->rid);
		/* it is actually the designer's choice how the tasks not 
		   successfully partitioned are scheduled. */
		set_scheduler(rt, RESCH_SCHED_FAIR, 0);
	}
}

static int __init fpff_init(void)
{
	printk(KERN_INFO "FP-FF: HELLO!\n");
	install_scheduler(task_run, NULL, NULL, NULL);
	
	return 0;
}

static void __exit fpff_exit(void)
{
	printk(KERN_INFO "FP-FF: GOODBYE!\n");
	uninstall_scheduler();
}

module_init(fpff_init);
module_exit(fpff_exit);
