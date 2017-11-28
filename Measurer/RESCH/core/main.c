/*
 * main.c:		Copyright (C) Shinpei Kato, Yusuke Fujii
 *
 * Entry point of the RESCH core.
 * The RESCH core is a character device module.
 * User applications access the RESCH core through ioctl() & write().
 */

#if defined(USE_XENIAL) || defined(USE_VIVID_OR_OLDER)
#include <asm/uaccess.h>
#else
#include <linux/uaccess.h> 
#endif
#include <asm/current.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <resch-api.h>
#include <resch-config.h>
#include <resch-core.h>
#include <resch-gpu-core.h>
#include "component.h"
#include "event.h"
#include "main.h"
#include "reservation.h"
#include "sched.h"
#include "test.h"

/* device number. */
static dev_t dev_id;
/* char device structure. */
static struct cdev c_dev;

static inline unsigned long timespec_to_usecs(struct timespec *ts)
{
	unsigned long s = ts->tv_sec;
	unsigned long ns = ts->tv_nsec;
	return s * 1000000 + ns / 1000;
}

/* dummy function. */
static int resch_open(struct inode *inode, struct file *filp)
{
	return 0;
}

/* dummy function. */
static int resch_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/**
 * RESCH APIs are called through write() system call.
 * See api.h for details.
 * Do not use timespec_to_jiffies(), since it rounds up the value.
 */
static ssize_t resch_write
(struct file *file, const char *buf, size_t count, loff_t *offset)
{
	int res = RES_SUCCESS;
	struct api_struct a;
	unsigned long us;

	/* copy data to kernel buffer. */
	if (copy_from_user(&a, buf, count)) {
		printk(KERN_WARNING "RESCH: failed to copy data.\n");
		return -EFAULT;
	}

	switch (a.api) {
        /* PORT ROS: ROS APIs for real-time scheduling. */
    case API_SET_NODE:
        res = api_set_node(a.rid, a.arg.val);
        break;

    case API_START_CALLBACK:
        res = api_start_callback(a.rid, a.arg.val);
        break;

    case API_END_CALLBACK:
        res = api_end_callback(a.rid, a.arg.val);
        break;

		/* PORT I: preemptive periodic real-time scheduling.*/
	case API_INIT:
		res = api_init();
		break;

	case API_EXIT:
		res = api_exit(a.rid);
		break;

	case API_RUN:
		us = timespec_to_usecs(&a.arg.ts);
		res = api_run(a.rid, usecs_to_jiffies(us));
		break;

	case API_WAIT_PERIOD:
		res = api_wait_for_period(a.rid);
		break;

	case API_WAIT_INTERVAL:
		us = timespec_to_usecs(&a.arg.ts);
		res = api_wait_for_interval(a.rid, usecs_to_jiffies(us));
		break;

	case API_SET_PERIOD:
		us = timespec_to_usecs(&a.arg.ts);
		res = api_set_period(a.rid, usecs_to_jiffies(us));
		break;

	case API_SET_DEADLINE:
		us = timespec_to_usecs(&a.arg.ts);
		res = api_set_deadline(a.rid, usecs_to_jiffies(us));
		break;

	case API_SET_WCET:
		us = timespec_to_usecs(&a.arg.ts);
		res = api_set_wcet(a.rid, us);
		break;

	case API_SET_RUNTIME:
		us = timespec_to_usecs(&a.arg.ts);
		res = api_set_runtime(a.rid, us);
		break;

	case API_SET_PRIORITY:
		res = api_set_priority(a.rid, a.arg.val);
		break;

	case API_SET_SCHEDULER:
		res = api_set_scheduler(a.rid, a.arg.val);
		break;

	case API_BACKGROUND:
		res = api_background(a.rid);
		break;

		/* PORT II: event-driven asynchrous scheduling.*/
	case API_SLEEP:
		us = timespec_to_usecs(&a.arg.ts);
		res = api_sleep(a.rid, usecs_to_jiffies(us));
		break;

	case API_SUSPEND:
		res = api_suspend(a.rid);
		break;

	case API_WAKE_UP:
		res = api_wake_up(a.arg.val);
		break;

		/* PORT III: reservation-based scheduling.*/
	case API_RESERVE_START:
		us = timespec_to_usecs(&a.arg.ts);
		res = api_reserve_start(a.rid, usecs_to_jiffies(us), false);
		break;

	case API_RESERVE_START_XCPU:
		us = timespec_to_usecs(&a.arg.ts);
		res = api_reserve_start(a.rid, usecs_to_jiffies(us), true);
		break;

	case API_RESERVE_STOP:
		res = api_reserve_stop(a.rid);
		break;

	case API_RESERVE_EXPIRE:
		res = api_reserve_expire(a.rid);
		break;

	case API_SERVER_RUN:
		res = api_server_run(a.rid);
		break;

		/* PORT IV.*/
		/* PORT V: hierarchical scheduling.*/
	case API_COMPONENT_CREATE:
		res = api_component_create();
		break;

	case API_COMPONENT_DESTROY:
		res = api_component_destroy(a.arg.val);
		break;

	case API_COMPOSE:
		res = api_compose(a.rid, a.arg.val);
		break;

	case API_DECOMPOSE:
		res = api_decompose(a.rid);
		break;

	default: /* illegal api identifier. */
		res = RES_ILLEGAL;
		printk(KERN_WARNING "RESCH: illegal API identifier.\n");
		break;
	}

	return res;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0) /* we don't believe LINUX_VERSION_CODE */
static int resch_ioctl(struct file *file,
					   unsigned int cmd,
					   unsigned long arg)
#else
static int resch_ioctl(struct inode *inode,
					   struct file *file,
					   unsigned int cmd,
					   unsigned long arg)
#endif
{
	unsigned long val;
	switch(cmd){
	    case GDEV_IOCTL_CTX_CREATE:
		return gsched_ctxcreate(arg);
	    case GDEV_IOCTL_LAUNCH:
		return gsched_launch(arg);
	    case GDEV_IOCTL_SYNC:
		return gsched_sync(arg);
	    case GDEV_IOCTL_CLOSE:
		return gsched_close(arg);
	    case GDEV_IOCTL_NOTIFY:
		return gsched_notify(arg);
	    case GDEV_IOCTL_SETCID:
		return gsched_setcid(arg);
	    case GDEV_IOCTL_GETDEV:
		return gsched_getdev(arg);
	    default:

		/* copy data to kernel buffer. */
		if (copy_from_user(&val, (long *)arg, sizeof(long))) {
		    printk(KERN_WARNING "RESCH: failed to copy data.\n");
		    return -EFAULT;
		}

		switch (cmd) {
		    case TEST_SET_SWITCH_COST:
			switch_cost= val;
			printk(KERN_INFO "RESCH: switch cost = %lu us\n", switch_cost);
			break;

		    case TEST_SET_RELEASE_COST:
			release_cost= val;
			printk(KERN_INFO "RESCH: release cost = %lu us\n", release_cost);
			break;

		    case TEST_SET_MIGRATION_COST:
			migration_cost = val;
			printk(KERN_INFO "RESCH: migration cost = %lu us\n", migration_cost);
			break;

		    case TEST_GET_RELEASE_COST:
			return test_get_release_cost(val);

		    case TEST_GET_MIGRATION_COST:
			return test_get_migration_cost(val);

		    case TEST_GET_RUNTIME:
			return test_get_runtime();

		    case TEST_GET_UTIME:
			return test_get_utime();

		    case TEST_GET_STIME:
			return test_get_stime();

		    case TEST_RESET_STIME:
			return test_reset_stime();
		    default:
			printk(KERN_WARNING "RESCH: illegal test command.\n");
			return RES_FAULT;
		}}

	return RES_SUCCESS;
}

static struct file_operations resch_fops = {
	.owner = THIS_MODULE,
	.open = resch_open, /* do nothing but must exist. */
	.release = resch_release, /* do nothing but must exist. */
	.read = NULL,
	.write = resch_write,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)
	.unlocked_ioctl = resch_ioctl
#else
	.ioctl = resch_ioctl
#endif
};

static int __init resch_init(void)
{
	int ret;

	printk(KERN_INFO "RESCH: HELLO!\n");

	/* get the device number of a char device. */
	ret = alloc_chrdev_region(&dev_id, 0, 1, MODULE_NAME);
	if (ret < 0) {
		printk(KERN_WARNING "RESCH: failed to allocate device.\n");
		return ret;
	}

	/* initialize the char device. */
	cdev_init(&c_dev, &resch_fops);

	/* register the char device. */
	ret = cdev_add(&c_dev, dev_id, 1);
	if (ret < 0) {
		printk(KERN_WARNING "RESCH: failed to register device.\n");
		return ret;
	}

#ifdef _RTXG_
	printk(KERN_INFO "RESCH: enable GPU scheduling\n");
	gsched_init();
#endif /* RTXG */

	sched_init();
	component_init();

	return 0;
}

static void __exit resch_exit(void)
{
	printk(KERN_INFO "RESCH: GOODBYE!\n");

#ifdef _RTXG_
	gsched_exit();
#endif /* RTXG */

	sched_exit();
	component_exit();

	/* delete the char device. */
	cdev_del(&c_dev);
	/* return back the device number. */
	unregister_chrdev_region(dev_id, 1);
}

module_init(resch_init);
module_exit(resch_exit);
