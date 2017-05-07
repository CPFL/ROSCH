/*
 * edf-wm.c:		Copyright (C) Shinpei Kato
 *
 * EDF with Window-constrained Migration (EDF-WM) scheduler.
 *
 * It is designed based on the algorithm presented in:
 * - Semi-Partitioned Scheduling of Sporadic Task Systems on Multiprocessors
 *   (Kato et al., ECRTS'2008).
 */

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/time.h>
#include <resch-core.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("EDF-WM Scheduler");
MODULE_AUTHOR("Shinpei Kato");

#define MODULE_NAME	"edf-wm"

#ifdef RESCH_HARD_RT
#define C wcet
#else /* RESCH_SOFT_RT */
#define C runtime
#endif
#define task_is_split(rt) (edf_wm_task[(rt)->rid].deadline > 0)
#define split_is_on_cpu(rt, cpu) (edf_wm_task[(rt)->rid].runtime[cpu] > 0)
#define edf_wm_task_is_on_cpu(rt, cpu)	\
	(task_is_on_cpu(rt, cpu) || split_is_on_cpu(rt, cpu))
#ifdef EDF_WM_FIRST_FIT
#define partition(rt) cpu_first_fit(rt)
#else
#define partition(rt) cpu_worst_fit(rt)
#endif

/* available options. */
//#define EDF_WM_FIRST_FIT
//#define EDF_WM_ALWAYS_SPLIT
//#define EDF_WM_FAIR_SPLIT

#ifndef SCHED_DEADLINE
#error "You need to compile the kernel with SCHED_DEADLINE configuration."
#endif

#ifndef CONFIG_AIRS
#define SCHED_FCBS 				0x20000000
#define SCHED_FCBS_NO_CATCH_UP	0x10000000
#define SCHED_EXHAUSTIVE 		0x08000000
#endif

/* task control block used by EDF-WM. */
typedef struct edf_wm_task_struct {
	int first_cpu;
	int last_cpu;
	resch_task_t *rt;
	unsigned long deadline;
	unsigned long runtime[NR_RT_CPUS];
	struct list_head migration_list;
	struct hrtimer migration_hrtimer;
	u64 next_release;
	u64 sched_deadline;
	u64 sched_split_deadline;
} edf_wm_task_t;
edf_wm_task_t edf_wm_task[NR_RT_TASKS];

struct kthread_struct {
	struct task_struct *task;
	struct list_head list;
	spinlock_t lock;
} kthread[NR_RT_CPUS];

static void request_migration(edf_wm_task_t *et, int cpu_dst)
{
	unsigned long flags;
	resch_task_t *hp;

	INIT_LIST_HEAD(&et->migration_list);

	/* insert the task to the waiting list for the migration thread. */
	spin_lock_irqsave(&kthread[cpu_dst].lock, flags);
	list_add_tail(&et->migration_list, &kthread[cpu_dst].list);
	spin_unlock(&kthread[cpu_dst].lock);

	/* wake up the migration thread running on the destination CPU. */
	wake_up_process(kthread[cpu_dst].task);
	et->rt->task->state = TASK_UNINTERRUPTIBLE;
	set_tsk_need_resched(et->rt->task);
	local_irq_restore(flags);

	active_queue_lock(cpu_dst, &flags);
	hp = active_highest_prio_task(cpu_dst);
	if (hp) {
		set_tsk_need_resched(hp->task);
	}
	active_queue_unlock(cpu_dst, &flags);
	smp_send_reschedule(cpu_dst);
}

static int start_window_timer(edf_wm_task_t *et)
{
	hrtimer_set_expires(&et->migration_hrtimer, ns_to_ktime(et->next_release));
	hrtimer_start_expires(&et->migration_hrtimer, HRTIMER_MODE_ABS);

	return hrtimer_active(&et->migration_hrtimer);
}

static enum hrtimer_restart window_timer(struct hrtimer *timer)
{
	edf_wm_task_t *et = container_of(timer, edf_wm_task_t, migration_hrtimer);
	int cpu;

	for (cpu = smp_processor_id() + 1; cpu < NR_RT_CPUS; cpu++) {
		if (et->runtime[cpu] > 0) {
			request_migration(et, cpu);
			break;
		}
	}

	return HRTIMER_NORESTART;
}

static void init_window_timer(struct hrtimer *timer)
{
	hrtimer_init(timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
	timer->function = window_timer;
}

static void clear_split_task(resch_task_t *rt)
{
	int cpu;
	edf_wm_task_t *et = &edf_wm_task[rt->rid];

	et->first_cpu = RESCH_CPU_UNDEFINED;
	et->last_cpu = RESCH_CPU_UNDEFINED;
	et->rt = NULL;
	et->deadline = 0;
	for (cpu = 0; cpu < NR_RT_CPUS; cpu++) {
		et->runtime[cpu] = 0;
	}
}

static inline unsigned long max_ulong(unsigned long x, unsigned long y)
{
	return x > y ? x : y;
}

static inline unsigned long floor_ulong(unsigned long x, unsigned long y)
{
	if (x >= 0) {
		return (unsigned long) (x / y);
	}
	else {
		if (x % y == 0)
			return (unsigned long)(x / y);
		else
			return (unsigned long)(x / y) - 1;
	}
}

static inline unsigned long util(resch_task_t *p)
{
	return 100 * p->C / jiffies_to_usecs(p->period);
}

static inline unsigned long util_split(resch_task_t *p, int cpu)
{
	return 100 * edf_wm_task[p->rid].runtime[cpu] / jiffies_to_usecs(p->period);
}

static unsigned long util_cpu(int cpu)
{
	resch_task_t *p;
	unsigned long u = 0;

	/* compute for tasks assigned to @cpu. */
	p = global_highest_prio_task();
	while (p) {
		if (split_is_on_cpu(p, cpu)) {
			u += util_split(p, cpu);
		}
		else if (task_is_on_cpu(p, cpu)) {
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
		if (split_is_on_cpu(p, cpu)) {
			u[p->cpu_id] += util_split(p, cpu);
		}
		else if (task_is_on_cpu(p, cpu)) {
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
	unsigned long us = sched_overhead_cpu(cpu, L) + sched_overhead_task(rt, L);
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

	/* compute for tasks assigned to @cpu. */
	p = global_highest_prio_task();
	while (p) {
		if (split_is_on_cpu(p, cpu)) {
			x += max_ulong(0, p->period - edf_wm_task[p->rid].deadline) * 
				util_split(p, cpu);
			u += util_split(p, cpu);
			d = max_ulong(d, edf_wm_task[p->rid].deadline);
		}
		else if (task_is_on_cpu(p, cpu)) {
			x += max_ulong(0, p->period - p->deadline) * util(p);
			u += util(p);
			d = max_ulong(d, p->deadline);
		}
		p = global_next_prio_task(p);
	}

	/* compute for @rt. */
	if (split_is_on_cpu(rt, cpu)) {
		unsigned long deadline = edf_wm_task[rt->rid].deadline;
		x += max_ulong(0, rt->period - deadline) * util_split(rt, cpu);
		u += util_split(rt, cpu);
		d = max_ulong(d, deadline);
	}
	else {
		x += max_ulong(0, rt->period - rt->deadline) * util(rt);
		u += util(rt);
		d = max_ulong(d, rt->deadline);
	}

	/* if total utilization is equal to 100%, just return the max deadline.
	   dbf_test() will anyway reject the given task. */
	if (100 == u) {
		return d;
	}
	else if (100 < u) {
		return 0;
	}
	else {
		return max_ulong(x / (100 - u), d);
	}
}

/**
 * compute the demand bound of @rt in the given interval. 
 */
static unsigned long dbf_task(resch_task_t *rt, unsigned long L, int cpu)
{
	if (split_is_on_cpu(rt, cpu)) {
		unsigned long jf;
		edf_wm_task_t *et = &edf_wm_task[rt->rid];
		if (L < et->deadline) {
			return 0;
		}
		jf = max_ulong(0, (L - et->deadline) / rt->period + 1) * 
			et->runtime[cpu];
		return jf;
	}
	else {
		unsigned long us;
		if (L < rt->deadline) {
			return 0;
		}
		us = max_ulong(0, (L - rt->deadline) / rt->period + 1) * rt->C;
		return usecs_to_jiffies(us);
	}
}

/**
 * compute the total demand bound on @cpu in the given interval. 
 */
static inline unsigned long dbf_cpu(unsigned long L, int cpu)
{
	resch_task_t *p;
	unsigned long dbf_sum = 0;

	p = global_highest_prio_task();
	while (p) {
		if (edf_wm_task_is_on_cpu(p, cpu)) {
			dbf_sum += dbf_task(p, L, cpu);
		}
		p = global_next_prio_task(p);
	}

	return dbf_sum;
}

/**
 * schedulability test based on the demand bound function (DBF). 
 */
static inline int dbf_test(int cpu , resch_task_t *rt)
{
	resch_task_t *p;
	unsigned long L, Lmax;

	/* if the execution time is zero, it is obviously schedulable. */
	if (rt->C == 0) {
		return true;
	}

	/* length of dbf test. */
	if ((Lmax = dbf_len(cpu, rt)) == 0) {
		return false;
	}

	/* check for deadlines of tasks assigned to @cpu. */
	p = global_highest_prio_task();
	while (p) {
		if (split_is_on_cpu(p, cpu)) {
			L = edf_wm_task[p->rid].deadline;
		}
		else if (task_is_on_cpu(p, cpu)) {
			L = p->deadline;
		}
		else {
			p = global_next_prio_task(p);
			continue;
		}

		while (L <= Lmax) {
			if (dbf_cpu(L, cpu) + dbf_task(rt, L, cpu) + 
				overhead(rt, L, cpu) > L) {
				return false;
			}
			L += p->period;
		}
		p = global_next_prio_task(p);
	}

	/* check for deadlines of @rt. */
	L = rt->deadline;
	while (L <= Lmax) {
		if (dbf_cpu(L, cpu) + dbf_task(rt, L, cpu) + 
			overhead(rt, L, cpu) > L) {
			return false;
		}
		L += rt->period;
	}

	return true;
}

#ifdef EDF_WM_FIRST_FIT
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
 * compute the runtime for each split piece of the given task.
 */
static int set_split_runtime(resch_task_t *rt, int cpu)
{
	unsigned long L, Lmax = ~(0UL);
	unsigned long remain, runtime, runtime_min;
	resch_task_t *p;
	edf_wm_task_t *et = &edf_wm_task[rt->rid];

	runtime_min = (100 - util_cpu(cpu)) * rt->period / 100;

	/* check for deadlines of tasks assigned to @cpu. */
	p = global_highest_prio_task();
	while (p) {
		if (split_is_on_cpu(p, cpu)) {
			L = et->deadline;
		}
		else if (task_is_on_cpu(p, cpu)) {
			L = p->deadline;
		}
		else {
			p = global_next_prio_task(p);
			continue;
		}

		while (L <= Lmax) {
			remain = L - (dbf_cpu(L, cpu) + overhead(rt, L, cpu));
			runtime = remain / (floor_ulong(L - et->deadline, rt->period) + 1);
			/* renew runtime and reduce Lmax. */
			if (runtime <= runtime_min) {
				runtime_min = runtime;
				if (runtime_min == 0) {
					goto out;
				}
				et->runtime[cpu] = runtime_min;
				Lmax = dbf_len(cpu, rt);
				et->runtime[cpu] = 0;
			}
			L += p->period;
		}
		p = global_next_prio_task(p);
	}

	/* check for deadlines of @rt. */
	L = et->deadline;
	while (L <= Lmax) {
		remain = L - (dbf_cpu(L, cpu) + overhead(rt, L, cpu));
		runtime = remain / (floor_ulong(L - et->deadline, rt->period) + 1);
		/* renew runtime and reduce Lmax. */
		if (runtime <= runtime_min) {
			runtime_min = runtime;
			if (runtime == 0) {
				goto out;
			}
			et->runtime[cpu] = runtime_min;
			Lmax = dbf_len(cpu, rt);
			et->runtime[cpu] = 0;
		}
		L += rt->period;
	}

 out:
	et->runtime[cpu] = runtime_min;
	return true;
}

/**
 * sort the the CPU ID in decreasing order of runtimes.
 */
static void sort_runtime(unsigned long runtime[], int cpu_id[])
{
	int i, j;

	for (i = 0; i < NR_RT_CPUS; i++) {
		cpu_id[i] = i;
	}
	for (i = 0; i < NR_RT_CPUS - 1; i++) {
		for (j = NR_RT_CPUS - 1; j > i; j--) {
			if (runtime[cpu_id[j]] > runtime[cpu_id[j-1]]) {
				int tmp;
				tmp = cpu_id[j];
				cpu_id[j] = cpu_id[j-1];
				cpu_id[j-1] = tmp;
			}
		}
	}
}

/**
 * try to split the given task across multiple CPUs. 
 */
static int split(resch_task_t *rt)
{
	int i;
	int num;
	int cpu, cpu_id[NR_RT_CPUS];
	unsigned long sum_runtime;
	edf_wm_task_t *et;

	/* try splitting the task. */
	num = 2;
	et = &edf_wm_task[rt->rid];
 try_split:
	et->deadline = rt->deadline / num;
	sum_runtime = 0;
	for (cpu = 0; cpu < NR_RT_CPUS; cpu++) {
		set_split_runtime(rt, cpu);
	}

	/* select the largest num values. */
	sort_runtime(et->runtime, cpu_id);
	for (i = 0; i < num; i++) {
		if (et->runtime[cpu_id[i]] > et->deadline) {
			et->runtime[cpu_id[i]] = et->deadline;
		}
		sum_runtime += et->runtime[cpu_id[i]];
		if (et->runtime[cpu_id[i]] > 0) {
			if (et->first_cpu == RESCH_CPU_UNDEFINED || 
				et->first_cpu > cpu_id[i]) {
				et->first_cpu = cpu_id[i];
			}
			if (et->last_cpu == RESCH_CPU_UNDEFINED || 
				et->last_cpu < cpu_id[i]) {
				et->last_cpu = cpu_id[i];
			}
		}
	}

	if (jiffies_to_usecs(sum_runtime) >= rt->C) {
#ifdef EDF_WM_FAIR_SPLIT
		unsigned long over = (jiffies_to_usecs(sum_runtime) - rt->C) / num;
		for (i = 0; i < num; i++) {
			et->runtime[cpu_id[i]] -= over;
		}
#else
		unsigned long over = jiffies_to_usecs(sum_runtime) - rt->C;
		et->runtime[et->last_cpu] -= usecs_to_jiffies(over);
#endif
		et->rt = rt;
		for (i = num; i < NR_RT_CPUS; i++) {
			et->runtime[cpu_id[i]] = 0;
		}
		return et->first_cpu;
	}

	/* try to split across more CPUs. */
	num++;
	if (num <= NR_RT_CPUS) {
		clear_split_task(rt);
		goto try_split;
	}

#ifdef EDF_WM_ALWAYS_SPLIT
	return et->first_cpu;
#else
	clear_split_task(rt);
	return RESCH_CPU_UNDEFINED;
	/* return cpu_low_util(); */
#endif

}

/**
 * assign the given task to a particular CPU.
 * if no CPUs can accept the task, it is just assigned the lowest priority.
 * note that the task has not yet been inserted into the global list.
 */
static void task_run(resch_task_t *rt)
{
	int cpu_dst;

	/* aperiodic tasks are just skipped. */
	if (!rt->period) {
		return;
	}

	/* first clear the EDF-WM properties. */
	clear_split_task(rt);

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
	rt->cpu_id = cpu_dst; /* this is safe. */
	global_list_up();

	/* if partitioning succeeded, migrate @p to the cpu. 
	   otherwise, do the default fair scheduling. */
	if (cpu_dst != RESCH_CPU_UNDEFINED) {
		if (task_is_split(rt)) {
			edf_wm_task_t *et = &edf_wm_task[rt->rid];
			unsigned long runtime_save = et->rt->runtime;
			et->sched_deadline = et->rt->task->dl.sched_deadline;
			et->rt->deadline = et->deadline;
			et->rt->runtime = jiffies_to_usecs(et->runtime[et->first_cpu]);
			/* change the deadline. */
			set_scheduler(et->rt, RESCH_SCHED_EDF, rt->prio);
			et->sched_split_deadline = et->rt->task->dl.sched_deadline;
			et->rt->runtime = runtime_save;

			printk(KERN_INFO "EDF-WM: task#%d is split across to CPU#%d-%d.\n", 
				   rt->rid, et->first_cpu, et->last_cpu);
		}
		else {
			if (rt->policy != RESCH_SCHED_EDF) {
				set_scheduler(rt, RESCH_SCHED_EDF, RESCH_PRIO_EDF);
			}
			printk(KERN_INFO "EDF-WM: task#%d is assigned to CPU#%d.\n", 
				   rt->rid, cpu_dst);
		}
		rt->migratory = false;
		migrate_task(rt, cpu_dst);
	}
	else {
		printk(KERN_INFO "EDF-WM: task#%d is not schedulable.\n", rt->rid);
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
		edf_wm_task_t *et = &edf_wm_task[rt->rid];
		/* save the next window. */
		et->next_release = rt->task->dl.deadline;
		/* run the timer for window-constrained migration. */
		init_window_timer(&et->migration_hrtimer);
		start_window_timer(et);
		/* this cannot be exhaustive. */
		rt->task->dl.flags &= ~SCHED_EXHAUSTIVE;
	}
}

static void job_complete(resch_task_t *rt)
{
	if (task_is_split(rt)) {
		edf_wm_task_t *et = &edf_wm_task[rt->rid];
		hrtimer_cancel(&et->migration_hrtimer);
		et->rt->task->dl.sched_deadline = et->sched_split_deadline;
		et->rt->task->dl.deadline = et->next_release;
		et->rt->task->dl.runtime = et->sched_split_deadline;
		if (smp_processor_id() != et->first_cpu) {
			migrate_task(rt, et->first_cpu);
		}
	}
}

static void migration_thread(void *__data)
{
	int cpu = (long) __data;
	edf_wm_task_t *et;
	struct timespec ts;

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
		et = list_first_entry(&kthread[cpu].list, 
							  edf_wm_task_t,
							  migration_list);
		list_del_init(&et->migration_list);
		spin_unlock_irq(&kthread[cpu].lock);

		/* account runtime. */
		jiffies_to_timespec(et->runtime[cpu], &ts);
		et->rt->task->dl.sched_runtime = timespec_to_ns(&ts);

		/* trace precise deadlines. */
		et->rt->deadline_time += et->deadline;
		et->rt->task->dl.sched_deadline = et->sched_split_deadline;
		et->rt->task->dl.deadline = et->next_release;
		et->next_release += et->sched_split_deadline;

		/* now let's migrate the task! */
		et->rt->task->dl.flags |= DL_NEW;
		migrate_task(et->rt, cpu);
		wake_up_process(et->rt->task);

		/* when the budget is exhausted, the deadline should be added by
		   et->sched_deadline but not by et->sched_split_deadline. */
		et->rt->task->dl.sched_deadline = et->sched_deadline;

		/* account runtime. */
		jiffies_to_timespec(et->runtime[cpu], &ts);
		et->rt->task->dl.runtime = timespec_to_ns(&ts);

		/* activate the timer for the next migration of this task. */
		if (et->last_cpu != cpu) {
			et->rt->task->dl.flags &= ~SCHED_EXHAUSTIVE;
			start_window_timer(et);
		}
		else {
			et->rt->task->dl.flags |= SCHED_EXHAUSTIVE;
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
										   "edf-wm-kthread");
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

static int __init edf_wm_init(void)
{
	printk(KERN_INFO "EDF-WM: HELLO!\n");
	install_scheduler(task_run, task_exit, job_start, job_complete);
	migration_thread_init();
	return 0;
}

static void __exit edf_wm_exit(void)
{
	printk(KERN_INFO "EDF-WM: GOODBYE!\n");
	uninstall_scheduler();
	migration_thread_exit();
}

module_init(edf_wm_init);
module_exit(edf_wm_exit);
