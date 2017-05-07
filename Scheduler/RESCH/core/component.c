#include <asm/current.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include "resch-api.h"
#include "resch-core.h"
#include "bitops.h"
#include "component.h"
#include "sched.h"

struct cid_map_struct cid_map;

/**
 * a bitmap for the component IDs used in hierarchical scheduling.
 * the same rule is applied as pid_map_struct.
 */
#define CID_MAP_LONG BITS_TO_LONGS(NR_RT_COMPONENTS)
struct cid_map_struct {
	unsigned long bitmap[CID_MAP_LONG];
	spinlock_t lock;
};

/**
 * component control block.
 */
struct component_struct {
	unsigned long priority;
	unsigned long period;
	unsigned long budget;
	struct semaphore sem;
	struct list_head task_list;
} component[NR_RT_COMPONENTS];

/**
 * API: create a new component in hierarchical scheduling. 
 * return the ID of the created component. 
 */
int api_component_create(void)
{
	int cid;

	spin_lock_irq(&cid_map.lock);
	/* get the available component ID. */
	if ((cid = resch_ffz(cid_map.bitmap, CID_MAP_LONG)) < 0) {
		cid = RES_FAULT;
		goto unlock;
	}
	__set_bit(cid, cid_map.bitmap);
 unlock:
	spin_unlock_irq(&cid_map.lock);

	/* initialize the component. */
	INIT_LIST_HEAD(&component[cid].task_list);

	return cid;
}

/**
 * API: destroy the given component in hierarchical scheduling. 
 */
int api_component_destroy(int cid)
{
	spin_lock_irq(&cid_map.lock);
	__clear_bit(cid, cid_map.bitmap);
	spin_unlock_irq(&cid_map.lock);

	return RES_SUCCESS;
}

/**
 * API: compose the current task into the given component. 
 */
int api_compose(int rid, int cid)
{
	resch_task_t *rt = resch_task_ptr(rid);
	if (cid >= NR_RT_COMPONENTS) {
		return RES_FAULT;
	}
	if (test_bit(cid, cid_map.bitmap)) {
		return RES_FAULT;
	}
	if (!list_empty(&rt->component_entry)) {
		return RES_FAULT;
	}

	list_add_tail(&rt->component_entry, 
				  &component[cid].task_list);
	return RES_SUCCESS;
}

/**
 * API: decompose the current task from the associated component. 
 */
int api_decompose(int rid)
{
	resch_task_t *rt = resch_task_ptr(rid);
	if (list_empty(&rt->component_entry)) {
		return RES_FAULT;
	}
	else {
		list_del_init(&rt->component_entry);
		return RES_SUCCESS;
	}
}

void component_init(void)
{
	int i;
	for (i = 0; i < CID_MAP_LONG; i++) {
		cid_map.bitmap[i] = 0;
	}
}

void component_exit(void)
{
}
