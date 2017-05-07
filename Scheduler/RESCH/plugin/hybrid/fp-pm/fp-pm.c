/*
 * fp-pm.c:			Copyright (C) Shinpei Kato
 *
 * Fixed-Priority with Priority Migration (FP-PM) scheduler.
 *
 * It is designed based on the algorithm presented in:
 * - Semi-Partitioned Fixed-Priority Scheduling on Multiprocessors
 *   (Kato et al., RTAS'2008).
 *
 * It is very important to remember that FP-PM may be inferior to FP-FF,
 * which is a parititioned Fixed-Priority with a first-fit CPU allocation.
 * This is because a task may be schedulable at runtime, even though the 
 * first-fit CPU allocation fails in FP-FF, while FP-PM essentially splits
 * the task across multiple CPUs.
 * This task splitting may incur timing violations due to uncertaintities.
 * Let's consider a simple example below.
 * We have six tasks {T1, T2, T3, T4, T5, T6}, and  only T4 is failed to 
 * be partitioned.
 * In FP-FF, T4 may be successfully scheduled at runtime, though it cannot
 * in terms of theoretical analysis.
 * Meanwhile, FP-PM splits T4 for instance across three CPUs.
 * As a result, the following tasks {T5, T6} will be affected by this task
 * splitting.
 * In this case, it may be better to just leave T4 with low priority.
 *
 * According to our experiments, FP-PM outperforms FP-FF in most cases, but
 * we have to remember that FP-PM may be inferior to FP-FF!
 *
 * Currently, the FP-PM scheduler does not support synchornization and self-
 * suspension. We do not guarantee performance when they are used.
 * We will however soon support these mechanisms.
 */

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <resch-core.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("FP-PM");
MODULE_AUTHOR("Shinpei Kato");

#define MODULE_NAME	"fp-pm"

#ifdef RESCH_HARD_RT
#define C wcet
#else /* RESCH_SOFT_RT */
#define C runtime
#endif

#define task_is_split(rt) \
	(fppm_task[(rt)->rid].first_cpu != fppm_task[(rt)->rid].last_cpu)
#define split_is_on_cpu(rt, cpu) (fppm_task[(rt)->rid].runtime[cpu] > 0)
#define fppm_task_is_on_cpu(rt, cpu)	\
	(task_is_on_cpu(rt, cpu) || split_is_on_cpu(rt, cpu))

#define FPPM_PRIO_MIGRATORY		(RESCH_PRIO_KTHREAD - 1)

/* task control block used by FP-PM. */
typedef struct fppm_task_struct {
	int first_cpu;
	int last_cpu;
	resch_task_t *rt;
	unsigned long runtime[NR_RT_CPUS];
	struct list_head migration_list;
	struct hrtimer migration_hrtimer;
} fppm_task_t;
fppm_task_t fppm_task[NR_RT_TASKS];

struct kthread_struct {
	struct task_struct *task;
	struct list_head list;
	spinlock_t lock;
} kthread[NR_RT_CPUS];

int cpu_available[NR_RT_CPUS] = {[0 ... NR_RT_CPUS-1] = true};

static void request_migration(fppm_task_t *ft, int cpu_dst)
{
	unsigned long flags;
	resch_task_t *hp;

	INIT_LIST_HEAD(&ft->migration_list);

	/* insert the task to the waiting list for the migration thread. */
	spin_lock_irqsave(&kthread[cpu_dst].lock, flags);
	list_add_tail(&ft->migration_list, &kthread[cpu_dst].list);
	spin_unlock(&kthread[cpu_dst].lock);

	/* wake up the migration thread running on the destination CPU. */
	wake_up_process(kthread[cpu_dst].task);
	ft->rt->task->state = TASK_UNINTERRUPTIBLE;
	set_tsk_need_resched(ft->rt->task);
	local_irq_restore(flags);

	active_queue_lock(cpu_dst, &flags);
	hp = active_highest_prio_task(cpu_dst);
	if (hp) {
		set_tsk_need_resched(hp->task);
	}
	active_queue_unlock(cpu_dst, &flags);
	smp_send_reschedule(cpu_dst);
}

static int start_migration_timer(fppm_task_t *ft, int cpu)
{
	u64 interval = ft->runtime[cpu] * 1000LL;
	hrtimer_set_expires(&ft->migration_hrtimer, ns_to_ktime(interval));
	hrtimer_start_expires(&ft->migration_hrtimer, HRTIMER_MODE_REL);

	return hrtimer_active(&ft->migration_hrtimer);
}

static enum hrtimer_restart migration_timer(struct hrtimer *timer)
{
	fppm_task_t *ft = container_of(timer, fppm_task_t, migration_hrtimer);
	int cpu;

	for (cpu = smp_processor_id() + 1; cpu < NR_RT_CPUS; cpu++) {
		if (ft->runtime[cpu] > 0) {
			request_migration(ft, cpu);
			break;
		}
	}

	return HRTIMER_NORESTART;
}

static void init_migration_timer(struct hrtimer *timer)
{
	hrtimer_init(timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer->function = migration_timer;
}

static inline void clear_split_task(resch_task_t *rt)
{
	int cpu;
	fppm_task[rt->rid].first_cpu = RESCH_CPU_UNDEFINED;
	fppm_task[rt->rid].last_cpu = RESCH_CPU_UNDEFINED;
	fppm_task[rt->rid].rt = NULL;
	for (cpu = 0; cpu < NR_RT_CPUS; cpu++) {
		fppm_task[rt->rid].runtime[cpu] = 0;
	}
}

/**
 * compute the worst-case response time of @rt on the given CPU.
 * this is an extension of the original response_time_analysis()
 * defined in resch.c in that it takes into accout the migratory tasks.
 * it also sets cpu_avaialble flags.
 * the global list must be locked.
 */
static unsigned long fppm_response_time_analysis(resch_task_t *rt, int cpu) 
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
		Pk = jiffies_to_usecs(hp->period);
		Ck = 0;

		if (split_is_on_cpu(hp, cpu)) {
			fppm_task_t *ft = &fppm_task[hp->rid];
			Ck = ft->runtime[cpu];
			/* this CPU becomes unavailable if not the last CPU of @hp */
			if (ft->last_cpu != cpu) {
				cpu_available[cpu] = false;
			}
		}
		else if (task_is_on_cpu(hp, cpu) && hp != rt) {
			Ck = hp->C;
		}

		if (Ck != 0) {
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
		Ri = fppm_response_time_analysis(rt, cpu);
		if (Ri > Di) {
			continue;
		}

		/* skip all higher-priority tasks, and get the first task whose
		   priority is less than or equal to @rt. */
		lp = global_prio_task(rt->prio);

		/* for the tasks with priorities lower than or equal to @rt. */
		success = true;
		while (lp) {
			if (!fppm_task_is_on_cpu(lp, cpu)) {
				lp = global_next_prio_task(lp);
				continue;
			}
			Rk = fppm_response_time_analysis(lp, cpu);
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
 * try to split the given task across multiple CPUs.
 */
static int split(resch_task_t *rt)
{
	int cpu;
	int F;
	unsigned long split_runtime, sum_split_runtime, tmp;
	unsigned long Di, Pi, Ci;
	unsigned long Rk, Dk;
	resch_task_t *lp;
	fppm_task_t *ft	= &fppm_task[rt->rid];

	/* deadline, period, and execution time. */
	Di = jiffies_to_usecs(rt->deadline);
	Pi = jiffies_to_usecs(rt->period);
	Ci = rt->C;

	sum_split_runtime = 0;
	for (cpu = 0; cpu < NR_RT_CPUS; cpu++) {
		if (cpu_available[cpu]) {
			split_runtime = 0;
			lp = global_highest_prio_task();
			while (lp) {
				if (!fppm_task_is_on_cpu(lp, cpu)) {
					lp = global_next_prio_task(lp);
					continue;
				}

				Rk = fppm_response_time_analysis(lp, cpu);
				Dk = jiffies_to_usecs(lp->deadline);
				lp = global_next_prio_task(lp);

				/* task @lp should be schedulable... */
				if (unlikely(Dk < Rk)) {
					split_runtime = 0;
					break;
				}

				/* compute the upper bound of execution capacity that 
				   can be assigned to task @p on each CPU. */
				F = Dk / Pi;
				if (F == 0) {
					tmp = (Dk - Rk) / (F + 1);
				}
				else {
					/* just reduce divisions as many as possible. */
					if (F * (Dk - Rk) >= (F * Pi - Rk) * (F + 1)) {
						tmp = (Dk - Rk) / (F + 1);
					}
					else {
						tmp = Pi - Rk / F;
					}
				}
				
				/* find the minimum value. */
				if (split_runtime > tmp || split_runtime == 0) {
					split_runtime = tmp;
				}
			}

			ft->runtime[cpu] = split_runtime;
			sum_split_runtime += split_runtime;

			/* set first cpu if necessary. */
			if (ft->first_cpu == RESCH_CPU_UNDEFINED && split_runtime > 0) {
				ft->first_cpu = cpu;
			}

			/* if successfully split, break a loop. */
			if (sum_split_runtime >= rt->C) {
				ft->runtime[cpu] -= (sum_split_runtime - rt->C);
				ft->last_cpu = cpu;
				break;
			}
		}
	}

	/* if @rt was successfully split, initialize as a migratory task.
	   also set the destination cpu by the first cpu. */
	if (ft->last_cpu != RESCH_CPU_UNDEFINED) {
		ft->rt = rt;
		return ft->first_cpu;
	}
	else {
		clear_split_task(rt);
		return RESCH_CPU_UNDEFINED;
	}
}

/**
 * assign the given task to a particular CPU.
 * if no CPUs can accept the task, its is split across multiple CPUs.
 * note that the task has not yet been inserted into the task list.
 */
static void task_run(resch_task_t *rt)
{
	int cpu, cpu_dst;

	/* aperiodic tasks are just skipped. */
	if (!rt->period) {
		return ;
	}

	/* first clear the FP-PM properties. */
	clear_split_task(rt);

	/* reset cpus availability. */
	for (cpu = 0; cpu < NR_RT_CPUS; cpu++) {
		cpu_available[cpu] = true;
	}

	/* this is going to be a big lock! */
	global_list_down();

	/* try partitioning. */
	if ((cpu_dst = partition(rt)) != RESCH_CPU_UNDEFINED) {
		/* schedulable. */
		goto out; 
	}

	/* try splitting. */
	cpu_dst = split(rt);

 out:
	rt->cpu_id = cpu_dst;
	global_list_up();

	/* if partitioning succeeded, migrate @p to the cpu. 
	   otherwise, do the default fair scheduling. */
	if (cpu_dst != RESCH_CPU_UNDEFINED) {
		if (task_is_split(rt)) {
			fppm_task_t *ft = &fppm_task[rt->rid];
			set_scheduler(ft->rt, RESCH_SCHED_FP, FPPM_PRIO_MIGRATORY);
			printk(KERN_INFO "FP-PM: task#%d is split across to CPU#%d-%d.\n", 
				   rt->rid, ft->first_cpu, ft->last_cpu);
		}
		else {
			if (rt->policy != RESCH_SCHED_FP) {
				set_scheduler(rt, RESCH_SCHED_FP, rt->prio);
			}
			printk(KERN_INFO "FP-PM: task#%d is assigned to CPU#%d.\n", 
				   rt->rid, cpu_dst);
		}
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

static void task_exit(resch_task_t *rt)
{
	clear_split_task(rt);
}

static void job_start(resch_task_t *rt)
{
	if (task_is_split(rt)) {
		fppm_task_t *ft = &fppm_task[rt->rid];
		int cpu = smp_processor_id();
		init_migration_timer(&ft->migration_hrtimer);
		start_migration_timer(ft, cpu);
	}
}

static void job_complete(resch_task_t *rt)
{
	if (task_is_split(rt)) {
		fppm_task_t *ft = &fppm_task[rt->rid];
		hrtimer_cancel(&ft->migration_hrtimer);
		if (smp_processor_id() != ft->first_cpu) {
			/* FIX ME: we should not use migrate_task() when we want to
			   traces preemption, since the task will complete. */
			migrate_task(rt, ft->first_cpu);
		}
	}
}

static void migration_thread(void *__data)
{
	int cpu = (long) __data;
	fppm_task_t *ft;

	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		spin_lock_irq(&kthread[cpu].lock);
		if (list_empty(&kthread[cpu].list)) {
			spin_unlock_irq(&kthread[cpu].lock);
			schedule();
			set_current_state(TASK_INTERRUPTIBLE);
			continue;
		}

		/* get a task in the list by fifo. */
		ft = list_first_entry(&kthread[cpu].list, 
							  fppm_task_t,
							  migration_list);
		list_del_init(&ft->migration_list);
		spin_unlock_irq(&kthread[cpu].lock);

		/* now let's migrate the task! */
		migrate_task(ft->rt, cpu);
		wake_up_process(ft->rt->task);

		/* activate the timer for the next migration of this task. */
		if (ft->last_cpu != cpu) {
			start_migration_timer(ft, cpu);
		}
	}
}

static void migration_thread_init(void)
{
	int cpu;
	struct sched_param sp = { .sched_priority = RESCH_PRIO_KTHREAD };

	for (cpu = 0; cpu < NR_RT_CPUS; cpu++) {
		INIT_LIST_HEAD(&kthread[cpu].list);
		kthread[cpu].task = kthread_create((void*)migration_thread,
										   (void*)(long)cpu,
										   "fp-pm-kthread");
		if (kthread[cpu].task != ERR_PTR(-ENOMEM)) {
			kthread_bind(kthread[cpu].task, cpu);
			sched_setscheduler(kthread[cpu].task, SCHED_FIFO, &sp);
			wake_up_process(kthread[cpu].task);
		}
		else {
			kthread[cpu].task = NULL;
		}
	}
}	

static void migration_thread_exit(void)
{
	int cpu;
	for (cpu = 0; cpu < NR_RT_CPUS; cpu++) {
		if (kthread[cpu].task) {
			kthread_stop(kthread[cpu].task);
		}
	}
}

static int __init fppm_init(void)
{
	printk(KERN_INFO "FP-PM: HELLO!\n");
	install_scheduler(task_run, task_exit, job_start, job_complete);
	migration_thread_init();

	return 0;
}

static void __exit fppm_exit(void)
{
	printk(KERN_INFO "FP-PM: GOODBYE!\n");
	uninstall_scheduler();
	migration_thread_exit();
}

module_init(fppm_init);
module_exit(fppm_exit);

