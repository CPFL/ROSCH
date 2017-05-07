#ifndef __RESCH_API_H__
#define __RESCH_API_H__

#include <linux/sched.h>

/**
 * device name.
 */
#define RESCH_DEVNAME	"/dev/resch"

/**
 * API command numbers.
 * the APIs are compliant with Portable Real-Time API (PORT).
 */
/* ROS APIs for real-time scheduling. */
#define API_ROS_OFFSET 0
#define API_SET_NODE	    API_ROS_OFFSET + 0
#define API_START_CALLBACK	API_ROS_OFFSET + 1
#define API_END_CALLBACK    API_ROS_OFFSET + 2

#define API_PORT1_OFFSET	API_ROS_OFFSET + 3
/* PORT-I: preemptive periodic real-time scheduling. */
#define API_INIT	 		API_PORT1_OFFSET + 0
#define API_EXIT			API_PORT1_OFFSET + 1
#define API_RUN 			API_PORT1_OFFSET + 2
#define API_WAIT_PERIOD		API_PORT1_OFFSET + 3
#define API_WAIT_INTERVAL	API_PORT1_OFFSET + 4
#define API_SET_PERIOD		API_PORT1_OFFSET + 5
#define API_SET_DEADLINE 	API_PORT1_OFFSET + 6
#define API_SET_WCET		API_PORT1_OFFSET + 7
#define API_SET_RUNTIME		API_PORT1_OFFSET + 8
#define API_SET_PRIORITY	API_PORT1_OFFSET + 9
#define API_SET_SCHEDULER	API_PORT1_OFFSET + 10
#define API_BACKGROUND		API_PORT1_OFFSET + 11

#define API_PORT2_OFFSET	API_PORT1_OFFSET + 12
/* PORT-II: event-driven asynchrous scheduling. */
#define API_SLEEP			API_PORT2_OFFSET + 0
#define API_SUSPEND			API_PORT2_OFFSET + 1
#define API_WAKE_UP			API_PORT2_OFFSET + 2

#define API_PORT3_OFFSET	API_PORT2_OFFSET + 3
/* PORT-III: reservation-based scheduling. */
#define API_RESERVE_START		API_PORT3_OFFSET + 0
#define API_RESERVE_START_XCPU	API_PORT3_OFFSET + 1
#define API_RESERVE_STOP		API_PORT3_OFFSET + 2
#define API_RESERVE_EXPIRE		API_PORT3_OFFSET + 3
#define API_SERVER_RUN			API_PORT3_OFFSET + 4

#define API_PORT4_OFFSET	API_PORT3_OFFSET + 5
/* PORT-IV: resource sharing. */
#define API_DOWN_MUTEX		API_PORT4_OFFSET + 0
#define API_UP_MUTEX		API_PORT4_OFFSET + 1

#define API_PORT5_OFFSET	API_PORT4_OFFSET + 2
/* PORT-V: hierarchical scheduling. */
#define API_COMPONENT_CREATE	API_PORT5_OFFSET + 0
#define API_COMPONENT_DESTROY	API_PORT5_OFFSET + 1
#define API_COMPONENT_PERIOD	API_PORT5_OFFSET + 2
#define API_COMPONENT_BUDGET	API_PORT5_OFFSET + 3
#define API_COMPOSE				API_PORT5_OFFSET + 4
#define API_DECOMPOSE			API_PORT5_OFFSET + 5

/* test command numbers. */
#define TEST_SET_SWITCH_COST	101
#define TEST_SET_RELEASE_COST	102
#define TEST_SET_MIGRATION_COST	103
#define TEST_GET_RELEASE_COST	104
#define TEST_GET_MIGRATION_COST	105
#define TEST_GET_RUNTIME		106
#define TEST_GET_UTIME			107
#define TEST_GET_STIME			108
#define TEST_RESET_STIME			109

/* ioctl command numbers  */

/* result values. they should be non-negative! */
#define RES_SUCCESS	0	/* sane result. */
#define RES_FAULT	1	/* insane result. */
#define RES_ILLEGAL	2	/* illegal operations. */
#define RES_MISS	3 	/* deadline miss. */

union api_arg_union {
	int val;
	struct timespec ts;
};

struct api_struct {
	int api;
	int rid;
	union api_arg_union arg;
};

struct api_user_struct {
	int api;
	int rid;
};

struct api_int_user_struct {
	int api;
	int rid;
	int val;
};

struct api_ts_user_struct {
	int api;
	int rid;
	struct timespec ts;
};

#endif
