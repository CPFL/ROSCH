/*
 * edf-ff.c:		Copyright (C) Shinpei Kato
 *
 * EDF with First-Fit partitioning (EDF-FF) scheduler.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <resch-core.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("EDF-FF Scheduler");
MODULE_AUTHOR("Shinpei Kato");

#define MODULE_NAME	"edf-ff"

static inline unsigned long max_ulong(unsigned long x, unsigned long y)
{
	return x > y ? x : y;
}

static inline unsigned long util(resch_task_t *p)
{
	return 100 * p->wcet / jiffies_to_usecs(p->period);
}

static unsigned long util_cpu(int cpu)
{
	resch_task_t *p;
	unsigned long u = 0;

	/* compute for tasks assigned to @cpu. */
	p = global_highest_prio_task();
	while (p) {
		if (task_is_on_cpu(p, cpu)) {
			u += util(p);
		}
		p = global_next_prio_task(p);
	}

	return u;
}

static inline int cpu_low_util(void)
{
	int cpu, cpu_low;
	resch_task_t *p;
	unsigned long u[NR_RT_CPUS+1] = {[0 ... NR_RT_CPUS] = 0};

	p = global_highest_prio_task();
	while (p) {
		if (task_is_on_cpu(p, cpu)) {
			u[p->cpu_id] += util(p);
		}
		p = global_next_prio_task(p);
	}
	cpu_low = 0;
	for (cpu = 1; cpu < NR_RT_CPUS; cpu++) {
		if (u[cpu] < u[cpu_low]) {
			cpu_low = cpu;
		}
	}
	return cpu_low;
}

/**
 * return the scheduling overhead by jiffies in the given interval.
 * the global list must be locked.
 */
static unsigned long overhead(resch_task_t *rt, unsigned long L, int cpu)
{
	unsigned long us = sched_overhead_cpu(cpu, L) +  sched_overhead_task(rt, L);
	return usecs_to_jiffies(us); /* always round up. */
}

/**
 * return the time length by jiffies, during which DBF should be tested 
 * on the give CPU.
 * the global list must be locked.
 */
static unsigned long dbf_len(int cpu, resch_task_t *rt)
{
	resch_task_t *p;
	unsigned long x = 0, u = 0, d = 0;

	/* compute for the tasks assigned to @cpu. */
	p = global_highest_prio_task();
	while (p) {
		if (!task_is_on_cpu(p, cpu)) {
			p = global_next_prio_task(p);
			continue;
		}
		x += max_ulong(0, p->period - p->deadline) * util(p);
		u += util(p);
		d = max_ulong(d, p->deadline);
		p = global_next_prio_task(p);
	}

	/* compute for @rt. */
	x += max_ulong(0, rt->period - rt->deadline) * util(rt);
	u += util(rt);
	d = max_ulong(d, rt->deadline);

	/* if total utilization exceeds 100%, just return the max deadline.
	   dbf_test() will anyway reject the given task. */
	if (100 <= u) {
		return d;
	}
	else {
		return max_ulong(x / (100 - u), d);
	}
}

/**
 * compute the demand bound of @rt by jiffies in the given interval. 
 */
static inline unsigned long dbf_task(resch_task_t *rt, unsigned long L)
{
	if (L < rt->deadline) {
		return 0;
	}
	else {
		unsigned long db_us;
		db_us = max_ulong(0, (L - rt->deadline) / rt->period + 1) * rt->wcet;
		return usecs_to_jiffies(db_us);
	}
}

/**
 * compute the total demand bound on @cpu in the given interval. 
 */
static inline unsigned long dbf_cpu(int cpu, unsigned long L)
{
	resch_task_t *p;
	unsigned long dbf_sum = 0;

	p = global_highest_prio_task();
	while (p) {
		if (task_is_on_cpu(p, cpu)) {
			dbf_sum += dbf_task(p, L);
		}
		p = global_next_prio_task(p);
	}

	return dbf_sum;
}

static inline int dbf_test(int cpu , resch_task_t *rt)
{
	resch_task_t *p;
	unsigned long L, Lmax;

	/* if wcet is zero, it is obviously schedulable. */
	if (rt->wcet == 0) {
		return true;
	}
	
	/* length of dbf test. */
	Lmax = dbf_len(cpu, rt);

	/* check for deadlines of tasks assigned to @cpu. */
	p = global_highest_prio_task();
	while (p) {
		if (!task_is_on_cpu(p, cpu)) {
			p = global_next_prio_task(p);
			continue;
		}

		L = p->deadline;
		while (L <= Lmax) {
			if (dbf_cpu(cpu, L) + dbf_task(rt, L) + overhead(rt, L, cpu) > L) {
				return false;
			}
			L += p->period;
		}

		p = global_next_prio_task(p);
	}

	/* check for deadlines of @rt. */
	L = rt->deadline;
	while (L <= Lmax) {
		if (dbf_cpu(cpu, L) + dbf_task(rt, L) + overhead(rt, L, cpu) > L) {
			return false;
		}
		L += rt->period;
	}

	return true;
}

#ifdef EDF_FIRST_FIT
#define partition(rt) cpu_first_fit(rt)
static int cpu_first_fit(resch_task_t *rt)
{
	int cpu;

	/* this is a first-fit assignment. */
	for (cpu = 0; cpu < NR_RT_CPUS; cpu++) {
		/* check the default CPU mask. */
		if (!cpu_isset(cpu, rt->task->cpus_allowed)) {
			continue;
		}

		/* demand bound function test. */
		if (dbf_test(cpu, rt)) {
			return cpu;
		}
	}

	return RESCH_CPU_UNDEFINED;
}
#else
#define partition(rt) cpu_worst_fit(rt)
static int cpu_worst_fit(resch_task_t *rt)
{
	int cpu, cpu_dst = RESCH_CPU_UNDEFINED;
	unsigned long u, u_dst = 100;

	/* this is a first-fit assignment. */
	for (cpu = 0; cpu < NR_RT_CPUS; cpu++) {
		/* check the default CPU mask. */
		if (!cpu_isset(cpu, rt->task->cpus_allowed)) {
			continue;
		}

		/* demand bound function test. */
		if (dbf_test(cpu, rt)) {
			u = util_cpu(cpu);
			if (u < u_dst) {
				u_dst = u;
				cpu_dst = cpu;
			}
		}
	}

	return cpu_dst;
}
#endif

/**
 * plugin function:
 * assign the given task to a particular CPU.
 * if no CPUs can accept the task, it is just assigned the lowest priority.
 * note that the task has not yet been inserted into the global list.
 */
void task_run(resch_task_t *rt)
{
	int cpu_dst;

	/* aperiodic tasks are just skipped. */
	if (!rt->period) {
		return;
	}

	/* this is going to be a big lock! */
	global_list_down();

	/* try partitioning. */
	if ((cpu_dst = partition(rt)) != RESCH_CPU_UNDEFINED) {
		/* schedulable. */
		goto out; 
	}

	/* the task is not guaranteed schedulable. */
	printk(KERN_INFO "EDF-FF: task#%d is not schedulable.\n", rt->rid);
	/* this is strict partitioning. if you remove this statement, you may
	   obtain schedulability improvement. */
	//cpu_dst = cpu_low_util(); 

 out:
	rt->cpu_id = cpu_dst; /* this is safe. */
	global_list_up();

	/* if partitioning succeeded, migrate @p to the cpu. 
	   otherwise, just do the default EDF scheduling. */
	if (cpu_dst != RESCH_CPU_UNDEFINED) {
		printk(KERN_INFO "EDF-FF: task#%d is assigned to CPU#%d.\n", 
			   rt->rid, cpu_dst);
		if (rt->policy != RESCH_SCHED_EDF) {
			set_scheduler(rt, RESCH_SCHED_EDF, rt->prio);
		}
		rt->migratory = false;
		migrate_task(rt, cpu_dst);
	}
	else {
		/* it is actually the designer's choice how the tasks not 
		   successfully partitioned are scheduled. */
		set_scheduler(rt, RESCH_SCHED_FAIR, 0);
	}
}

static int __init edf_ff_init(void)
{
	printk(KERN_INFO "EDF-FF: HELLO!\n");
	install_scheduler(task_run, NULL, NULL, NULL);
	
	return 0;
}

static void __exit edf_ff_exit(void)
{
	printk(KERN_INFO "EDF-FF: GOODBYE!\n");
	uninstall_scheduler();
}

module_init(edf_ff_init);
module_exit(edf_ff_exit);
