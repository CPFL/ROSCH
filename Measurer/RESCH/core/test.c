#include <resch-api.h>
#include <resch-core.h>
#include "sched.h"
#include "test.h"

unsigned long test_release_cost_us = 0;

static inline int test_nsecs_to_usecs(u64 ns)
{
	return (int) (ns / 1000);
}

#ifdef RESCH_HRTIMER
#define test_sleep(rt, interval) hrtimer_test_sleep(rt, interval)
static enum hrtimer_restart test_timer(struct hrtimer *timer)
{
	resch_task_t *rt = container_of(timer, resch_task_t, hrtimer_period);
	int cpu_now = smp_processor_id();
	u64 begin, end;

	begin = cpu_clock(cpu_now);
	wake_up_process(rt->task);
	end = cpu_clock(cpu_now);
	if (end > begin) {
		test_release_cost_us = test_nsecs_to_usecs(end - begin) + 1;
	}
	rt->task->state = TASK_RUNNING;
	return HRTIMER_NORESTART;
}

static int start_test_timer(resch_task_t *rt, u64 interval)
{
	hrtimer_set_expires(&rt->hrtimer_period, ns_to_ktime(interval));
	hrtimer_start_expires(&rt->hrtimer_period, HRTIMER_MODE_REL);
	return hrtimer_active(&rt->hrtimer_period);
}

static void init_test_timer(struct hrtimer *timer)
{
	hrtimer_init(timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer->function = test_timer;
}

void hrtimer_test_sleep(resch_task_t *rt, unsigned long interval)
{
	struct timespec ts;
	u64 ns;

	jiffies_to_timespec(interval, &ts);
	ns = timespec_to_ns(&ts);
	init_test_timer(&rt->hrtimer_period);
	start_test_timer(rt, ns);
	rt->task->state = TASK_UNINTERRUPTIBLE;
	schedule();
}
#else
#define test_sleep(rt, interval) timer_test_sleep(rt, interval)
static void test_timer(unsigned long __data)
{
	resch_task_t *rt = (resch_task_t*) __data;
	int cpu_now = smp_processor_id();
	u64 begin, end;

	begin = cpu_clock(cpu_now);
	wake_up_process(rt->task);
	end = cpu_clock(cpu_now);
	if (end > begin) {
		test_release_cost_us = test_nsecs_to_usecs(end - begin) + 1;
	}
	rt->task->state = TASK_RUNNING;
}

void timer_test_sleep(resch_task_t *rt, unsigned long interval)
{
	struct timer_list timer;

	setup_timer_on_stack(&timer, test_timer, (unsigned long)rt);
	mod_timer(&timer, jiffies + interval);
	current->state = TASK_UNINTERRUPTIBLE;
	schedule();
	del_timer_sync(&timer);
	destroy_timer_on_stack(&timer);
}
#endif

int test_get_release_cost(int rid)
{
	resch_task_t *rt = resch_task_ptr(rid);
	test_sleep(rt, rt->period);
	return test_release_cost_us;
}

int test_get_migration_cost(int rid)
{
	resch_task_t *rt = resch_task_ptr(rid);
	int cpu_now = smp_processor_id();
	int cpu_dst = (cpu_now + 1) % NR_RT_CPUS;
	u64 begin, end;

	begin = cpu_clock(cpu_dst);
	migrate_task(rt, cpu_dst);
	end = cpu_clock(cpu_dst);
	
	return test_nsecs_to_usecs(end - begin);
}

void test_reset_stime(void)
{
	return current->stime = 0;
}

int test_get_runtime(void)
{
	return current->utime + current->stime;
}

int test_get_utime(void)
{
	return current->utime;
}

int test_get_stime(void)
{
    printk("[%s]:stime=%ld, utime=%ld\n",__func__, current->stime, current->utime);
    return current->stime;
}

