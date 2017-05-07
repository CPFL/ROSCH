/*
 * Copyright (c) 2014 Yusuke Fujii
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <resch-gpu-core.h>
#include <resch-api.h>
#include <resch-config.h>
#include <resch-core.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include <linux/irq.h>
#include <linux/signal.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>

#include "bitops.h"

#define ENABLE_RESCH_INTERRUPT

#define CID_MAP_LONG 0x1f

#define GPU_VSCHED_BAND
// #define GPU_VSCHED_FIFO
// #define GPU_VSCHED_NULL

#if    defined(GPU_VSCHED_BAND)
#include "gpu_vsched_band.c"
struct gdev_vsched_policy *gdev_vsched = &gdev_vsched_band;
#elif  defined(GPU_VSCHED_FIFO)
#include "gpu_vsched_fifo.c"
struct gdev_vsched_policy *gdev_vsched = &gdev_vsched_fifo;
#else
#include "gpu_vsched_null.c"
struct gdev_vsched_policy *gdev_vsched = &gdev_vsched_null;
#endif
DEFINE_SPINLOCK( reschg_global_spinlock );

extern struct gdev_list list_irq_wakeup_head;
unsigned long cid_bitmap[100];
extern struct resch_irq_desc **resch_desc; 
extern struct gdev_list list_irq_wait_head;

struct rtxGhandle{
    uint32_t dev_id;
    uint32_t vdev_id;
    uint32_t cid;
    void *task;
    void *nvdesc;
    uint8_t sched_flag;
    uint32_t setcid;
    uint8_t sync_flag;
};

#define PICKUP_GPU_MIN 0x1
#define PICKUP_GPU_FIFO 0x2
#define PICKUP_GPU_ONE 0x0
#define PICKUP_GPU_SEQ 0x3
extern int gpu_count;
uint32_t vgid[4] = {0,0,0,0};

static inline void print_remaining_task(struct gdev_device *gdev){
    struct gdev_sched_entity *p;

    printk("=========\n");
    printk("[gdev#%d]remaining tasks are\n",gdev->id);
    gdev_list_for_each (p, &gdev->sched_com_list, list_entry_com) {
    	printk("[ctx%d]",p->ctx);
    }
    printk("\n");
    printk("=========\n");
}

/**
 * create a new scheduling entity.
 */
struct gdev_sched_entity* gdev_sched_entity_create(struct gdev_device *gdev, uint32_t cid)
{
    struct gdev_sched_entity *se;

    se = (struct gdev_sched_entity*)kmalloc(sizeof(*se),GFP_DMA);

    /* set up the scheduling entity. */
    se->gdev = gdev;
    se->task = NULL;//current;
    se->ctx = cid;
    
    se->prio = gdev->id * 30;
    // se->prio = current->static_prio;
    se->rt_prio = current->static_prio;
    se->launch_instances = 0;
    se->memcpy_instances = 0;
    gdev_list_init(&se->list_entry_com, (void*)se);
    gdev_list_init(&se->list_entry_mem, (void*)se);
    gdev_list_init(&se->list_wakeup_irq, (void*)se);
    gdev_list_init(&se->list_wait_irq, (void*)se);
    gdev_time_us(&se->last_tick_com, 0);
    gdev_time_us(&se->last_tick_mem, 0);
    se->wait_cond =0;
    atomic_set(&se->launch_count, 0);
    /*XXX*/
    sched_entity_ptr[cid] = se;
    return se;
}

#define O_H 0x1000
static inline void dl_runtime_reverse(struct gdev_sched_entity *se)
{
#ifdef SCHED_DEADLINE
    struct task_struct *task = se->task;
    long long delta;

    long long left_a;
    long long right_a;

    delta = sched_clock() - se->wait_time + O_H;
 
    if( delta > task->dl.runtime - 1000000){
	printk("Exhausted!\n");
	delta = task->dl.runtime - 1000000;
   }
 /*
    left_a = (task->dl.runtime >> 16) * (task->dl.dl_period >> 16);
    right_a = (task->dl.dl_runtime >> 16) * ((task->dl.deadline - sched_clock()) >> 16);
    printk("left:0x%lx, right:0x%lx\n",left_a, right_a);

    printk("dl_runtime =0x%lx, dl_period=0x%lx\n",task->dl.dl_runtime, task->dl.dl_period);
    printk("delta:0x%lx, runtime = 0x%lx, deadline:0x%lx, t:0x%lx\n", delta, task->dl.runtime, task->dl.deadline-sched_clock(), sched_clock());
   */ 
    task->dl.runtime -= delta;
#endif
}

static inline void dl_runtime_reserve(struct gdev_sched_entity *se)
{
#ifdef SCHED_DEADLINE
    se->dl_runtime = current->dl.runtime;
    se->dl_deadline = current->dl.deadline;
    se->wait_time = sched_clock();
#endif
}

void cpu_wq_sleep(struct gdev_sched_entity *se)
{
    struct task_struct *task = se->task;
    struct gdev_device *gdev = se->gdev;
    struct resch_irq_desc *desc = resch_desc;


    spin_lock_irq(&desc->release_lock);
    
    if(se->wait_cond != 0xDEADBEEF){
	se->wait_cond = 0xCAFE;
	dl_runtime_reserve(se);
	RESCH_G_DPRINT("Process GOTO SLEEP Ctx#0x%lx\n",se->ctx);
	spin_unlock_irq(&desc->release_lock);

	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
	
	spin_lock_irq(&desc->release_lock);
    }else{
	RESCH_G_DPRINT("Already fisnihed Ctx#%d\n",se->ctx);
	se->wait_cond = 0x0;
    }

    spin_unlock_irq(&desc->release_lock);
}

void cpu_wq_wakeup(struct gdev_sched_entity *se)
{
    struct task_struct *task = se->task;
    struct gdev_device *gdev = se->gdev;
    struct resch_irq_desc *desc = resch_desc;
    long long delta;

    spin_lock_irq(&desc->release_lock);
 
    if(se->wait_cond == 0xCAFE){
	

	dl_runtime_reverse(se);

	gdev_sched_wakeup(se);

	RESCH_G_DPRINT("Process Finish! Wakeup Ctx#0x%lx\n",se->ctx);
	se->wait_cond = 0x0;
    }else{
	se->wait_cond = 0xDEADBEEF;
	RESCH_G_DPRINT("Not have sleep it!%d\n",se->ctx);
    }
    spin_unlock_irq(&desc->release_lock);
}

void cpu_wq_wakeup_tasklet(unsigned long arg)
{
    struct gdev_sched_entity *se  = (struct gdev_sched_entity*)arg;
    cpu_wq_wakeup(se);
}


static inline int gpu_virtual_device_weight_set(struct gdev_device *dev, uint32_t weight)
{
    if(dev){
	dev->com_bw = weight;
	dev->mem_bw = weight;
    }
}

static uint32_t pick_up_next_phys_gpu(void)
{
    static uint32_t phys_id = 0;

#if 0/*FIFO*/
    phys_id++ % gpu_count;
#endif

    return phys_id;
}

static uint32_t pick_up_next_gpu(uint32_t phys_id, uint32_t flag)
{
    int i;
    int min = 99;
    struct gdev_device *phys = &phys_ds[phys_id];
    int __vgid = 0;
    struct gdev_device *p;
    static int seq_vgid = 0;

#ifdef ALLOC_VGPU_PER_ONETASK
    gdev_lock(&phys->sched_com_lock);
    while(gdev_vds[__vgid].parent != NULL)__vgid++;

    gdev_vds[__vgid].parent = phys;
    gsched_create_scheduler(&gdev_vds[__vgid]);

    gdev_list_for_each(p, &phys->list_entry_com, list_entry_com) {
	gpu_virtual_device_weight_set(p, 100/ __vgid);
    }
    gdev_unlock(&phys->sched_com_lock);
#else
	printk("[%s]:phys = 0x%lx\n",__func__,phys);
    if(!phys){
	return 0;
    }
    gdev_lock(&phys->sched_com_lock);
    if (!__vgid){
	switch (flag){
	    case PICKUP_GPU_MIN:
		printk("[%s]getvgid = ",__func__);
		for(i =0;i < GDEV_DEVICE_MAX_COUNT; i++){
		    if(gdev_vds[i].parent->id == phys_id){
			printk("..%d",i);
			if(min > gdev_vds[i].users){
			    printk(":%d",gdev_vds[i].users);
			    min = gdev_vds[i].users;
			    __vgid = i;
			}
		    }
		}
		break;

	    case PICKUP_GPU_ONE:
		for(i =0;i < GDEV_DEVICE_MAX_COUNT; i++){
		    if(gdev_vds[i].parent->id == phys_id)
			__vgid = i;
		    break;
		}
		/* if can't choose vgid, "not break" */
	    case PICKUP_GPU_SEQ:
		printk("select vgid %d \% %d\n",seq_vgid, GDEV_DEVICE_MAX_COUNT);
		__vgid = seq_vgid % GDEV_DEVICE_MAX_COUNT;
		seq_vgid++;
		break;
	    default:
		__vgid = 0;
		break;

	}
    }
    gdev_unlock(&phys->sched_com_lock);
#endif
    RESCH_G_DPRINT("RESCH select VGPU is %d\n", __vgid);
    return __vgid;
}


int gsched_ctxcreate(unsigned long arg)
{
    struct rtxGhandle *h = (struct rtxGhandle*)arg;
    struct gdev_device *dev;
    struct gdev_device *phys;

    uint32_t phys_id = h->dev_id;
    uint32_t vgid = h->vdev_id;
    uint32_t cid;


    if(h->vdev_id == 0xBEEF)
	vgid = pick_up_next_gpu(phys_id, PICKUP_GPU_MIN);

    dev = &gdev_vds[vgid];
    phys = dev->parent;

    if(phys){
retry:
	gdev_lock(&phys->global_lock);
	if(phys->users > GDEV_CONTEXT_MAX_COUNT ){/*GDEV_CONTEXT_LIMIT Check*/
	    printk("check context count\n");
	    gdev_unlock(&phys->global_lock);
	    schedule_timeout(100);
	    goto retry;
	}
	phys->users++;
	gdev_unlock(&phys->global_lock);
    }
    dev->users++;
 
    /* find empty entity  */
    spin_lock( &reschg_global_spinlock );
    
    if((cid = resch_ffz(cid_bitmap, CID_MAP_LONG)) < 0){
	printk("failed attached Gdev sched entity.\n");
	return 0;
    }
    __set_bit(cid, cid_bitmap);
    cid++; /* cid must not set zero, shifting one  */

    spin_unlock( &reschg_global_spinlock );

    h->cid = cid;
    h->setcid = 0;

    RESCH_G_PRINT("Opened RESCH_G, CTX#%d, GDEV=0x%lx current:0x%lx\n",cid,dev,current);
    struct gdev_sched_entity *se = gdev_sched_entity_create(dev, cid);
    
    return h->cid;
}

int gsched_getdev(unsigned long arg)
{
    struct rtxGhandle *h = (struct rtxGhandle*)arg;

    return pick_up_next_phys_gpu();
}

int gsched_setcid(unsigned long arg)
{
    struct rtxGhandle *h = (struct rtxGhandle*)arg;
    if((h->cid != h->setcid)){
	spin_lock( &reschg_global_spinlock );
	sched_entity_ptr[h->setcid] = sched_entity_ptr[h->cid];
	sched_entity_ptr[h->cid] = NULL;
	__clear_bit(h->cid, cid_bitmap);
	__set_bit(h->setcid, cid_bitmap);
	h->cid = h->setcid;
	spin_unlock( &reschg_global_spinlock );
	printk("Succesfull set cid%d !\n",h->setcid);
    }else{
	printk("Failed set cid%d. current cid is %d\n", h->setcid, h->cid);
    }
}

int gsched_launch(unsigned long arg)
{
    struct rtxGhandle *h = (struct rtxGhandle*)arg;
    struct gdev_sched_entity *se = sched_entity_ptr[h->cid];
 
    RESCH_G_DPRINT("Launch RESCH_G, CTX#%d\n",h->cid);
    gdev_schedule_compute(se);
 
#if 0
 
    if((h->sync_flag & 0xf0)){
	printk("se#%d is added to list_wait_irq\n",se->ctx);
	if(atomic_read(&se->launch_count) < 0){
	    atomic_set(&se->launch_count, 0);
	}
	atomic_inc(&se->launch_count);
	gdev_list_add_tail(&se->list_wait_irq, &list_irq_wait_head);
    }
#endif
}



int gsched_sync(unsigned long arg)
{
    static int count = 0;
    struct rtxGhandle *h = (struct rtxGhandle*)arg;
    struct gdev_sched_entity *se = sched_entity_ptr[h->cid];
    struct timer_list timer;
    uint32_t seq;

#if 0
    if(!h->nvdesc || (h->sync_flag & 0x10)){
//	spin_lock(&reschg_global_spinlock);
	if(se){
//	    spin_unlock(&reschg_global_spinlock);
	    RESCH_G_DPRINT("[%s[pid:%d]]: compute ctx#%d sleep in\n",__func__,current->pid, h->cid);
	   // gdev_sched_sleep(se);
	    RESCH_G_DPRINT("[%s[pid:%d]]: compute ctx#%d sleep out\n",__func__,current->pid, h->cid);
	    //spin_lock(&reschg_global_spinlock);
	    //se->task = NULL;
	    se->wait_irq_cond = 0;
//	    gdev_list_del(&se->list_wait_irq);
	    printk("se#%d was deleted by list_wait_irq\n",se->ctx);
	}
//	spin_unlock(&reschg_global_spinlock);
    }else{
#endif
	printk("[%s:%d]\n",__func__,__LINE__);
	//gdev_list_add_tail(&se->list_wakeup_irq, &list_irq_wakeup_head);
	struct gdev_device *gdev = &gdev_vds[h->vdev_id];
	if(gdev && gdev->sched_com_thread){

	    while(!wake_up_process(gdev->sched_com_thread)){
		printk("[%s]:failed wakeup com thread #%d\n",__func__,gdev->id);
		schedule_timeout_interruptible(50);
	    }
	
	}
#if 0
    }
#endif
}


int gsched_notify(unsigned long arg)
{
    struct rtxGhandle *h = (struct rtxGhandle*)arg;
    struct gdev_sched_entity *se = sched_entity_ptr[h->cid];
/*
    spin_lock( &reschg_global_spinlock );

    if(atomic_read(&se->launch_count) < 0){
	atomic_set(&se->launch_count, 0);
    }

    atomic_inc(&se->launch_count);
  
    spin_unlock( &reschg_global_spinlock );
*/
    printk("[%s:%d]\n",__func__,__LINE__);
    return ;
}

int gsched_close(unsigned long arg)
{
    struct rtxGhandle *h = (struct rtxGhandle*)arg;
    struct gdev_sched_entity *se = sched_entity_ptr[h->cid];
    struct gdev_device *dev = se->gdev;
    struct gdev_device *phys = dev->parent;
		   printk("[%s:%d]\n",__func__,__LINE__);

    spin_lock( &reschg_global_spinlock );
    gdev_sched_entity_destroy(se);
    sched_entity_ptr[h->cid] = NULL;
    __clear_bit(h->cid-1, cid_bitmap);
    spin_unlock( &reschg_global_spinlock );

    if(phys){
retry:
	gdev_lock(&phys->global_lock);
	phys->users--;
	gdev_unlock(&phys->global_lock);
    }
    dev->users--;

#ifdef ALLOC_VGPU_PER_ONETASK
    gsched_destroy_scheduler(dev);
    gpu_virtual_device_init(dev, 0,0,NULL);
#endif
}



/**
 * destroy the scheduling entity.
 */
void gdev_sched_entity_destroy(struct gdev_sched_entity *se)
{
    kfree(se);
}

/**
 * insert the scheduling entity to the priority-ordered compute list.
 * gdev->sched_com_lock must be locked.
 */
void __gdev_enqueue_compute(struct gdev_device *gdev, struct gdev_sched_entity *se)
{
    struct gdev_sched_entity *p;
printk("[%s:%d]\n",__func__,__LINE__);
    gdev_list_for_each (p, &gdev->sched_com_list, list_entry_com) {
	if (se->prio > p->prio) {
	    gdev_list_add_prev(&se->list_entry_com, &p->list_entry_com);
	    break;
	}
    }
    if (gdev_list_empty(&se->list_entry_com))
	gdev_list_add_tail(&se->list_entry_com, &gdev->sched_com_list);
#ifdef G_DEBUG
    print_remaining_task(&gdev_vds[0]);
#endif
}

/**
 * delete the scheduling entity from the priority-ordered compute list.
 * gdev->sched_com_lock must be locked.
 */
void __gdev_dequeue_compute(struct gdev_sched_entity *se)
{
    gdev_list_del(&se->list_entry_com);
#ifdef G_DEBUG
    print_remaining_task(&gdev_vds[0]);
#endif
}

/**
 * insert the scheduling entity to the priority-ordered memory list.
 * gdev->sched_mem_lock must be locked.
 */
void __gdev_enqueue_memory(struct gdev_device *gdev, struct gdev_sched_entity *se)
{
    struct gdev_sched_entity *p;

    gdev_list_for_each (p, &gdev->sched_mem_list, list_entry_mem) {
	if (se->prio > p->prio) {
	    gdev_list_add_prev(&se->list_entry_mem, &p->list_entry_mem);
	    break;
	}
    }
    if (gdev_list_empty(&se->list_entry_mem))
	gdev_list_add_tail(&se->list_entry_mem, &gdev->sched_mem_list);
#ifdef G_DEBUG
    print_remaining_task(&gdev_vds[0]);
#endif
}

/**
 * delete the scheduling entity from the priority-ordered memory list.
 * gdev->sched_mem_lock must be locked.
 */
void __gdev_dequeue_memory(struct gdev_sched_entity *se)
{
    gdev_list_del(&se->list_entry_mem);
    print_remaining_task(&gdev_vds[0]);
    print_remaining_task(&gdev_vds[1]);
}

void gdev_sched_sleep(struct gdev_sched_entity *se)
{

    spin_lock( &reschg_global_spinlock );
    if(!se || se->task == 0xDEAD){
	printk("[%s]:se is already wakeup\n",__func__);
	if(se)
	    se->task=NULL;
    spin_unlock( &reschg_global_spinlock );
	return -EINVAL;
    }
    printk("[%s:%d]sleep se#%d:0x%lx\n",__func__,task_tgid_vnr(current),se->ctx,se);
    struct task_struct *task = se->task = current;

#if 0
#ifdef SCHED_DEADLINE
    if(task->policy == SCHED_DEADLINE){
	cpu_wq_sleep(se);
    }else
    {	
#endif
#endif
    spin_unlock( &reschg_global_spinlock );
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
#if 0
#ifdef SCHED_DEADLINE
    }
#endif
#endif
}


int gdev_sched_wakeup(struct gdev_sched_entity *se)
{
    
    spin_lock( &reschg_global_spinlock );
    if(!se || !se->task){
	printk("[%s]:se does not sleep\n",__func__);
	se->task=0xDEAD;
    spin_unlock( &reschg_global_spinlock );
	return -EINVAL;
    }
    printk("[%s:%d]:wakeup se#%d:0x%lx\n", __func__,task_tgid_vnr(current) ,se->ctx,se);

    struct task_struct *task = se->task;

#if 1
#ifdef SCHED_DEADLINE
    if(task->policy == SCHED_DEADLINE){
	cpu_wq_wakeup(se);
    }
    else{
#endif
#endif
	spin_unlock( &reschg_global_spinlock );
	if(!wake_up_process(task)) {
	    schedule_timeout_interruptible(1);
	    if(!wake_up_process(task)){
		RESCH_G_PRINT("wakeup_failed......\n");
		return -EINVAL;
	    }
	}
	se->task = NULL;
#if 1
#ifdef SCHED_DEADLINE
    }
#endif
#endif
    return 1;
}

void* gdev_current_com_get(struct gdev_device *gdev)
{
    return gdev? gdev->current_com:NULL;
}

void gdev_current_com_set(struct gdev_device *gdev, void *com){
    gdev->current_com = com;
}

void gdev_lock_init(gdev_lock_t *p)
{
    spin_lock_init(&p->lock);
}

void gdev_lock(gdev_lock_t *p)
{
 //   printk("[%s]: lock_t:0x%lx\n",__func__,p);
    spin_lock_irq(&p->lock);
}

void gdev_unlock(gdev_lock_t *p)
{
  //  printk("[%s]: lock_t:0x%lx\n",__func__,p);
    spin_unlock_irq(&p->lock);
}

void gdev_lock_save(gdev_lock_t *p, unsigned long *pflags)
{
 //   printk("[%s]: lock_t:0x%lx\n",__func__,p);
    spin_lock_irqsave(&p->lock, *pflags);
}

void gdev_unlock_restore(gdev_lock_t *p, unsigned long *pflags)
{
 //   printk("[%s]: lock_t:0x%lx\n",__func__,p);
    spin_unlock_irqrestore(&p->lock, *pflags);
}

void gdev_lock_nested(gdev_lock_t *p)
{
  //  printk("[%s]: lock_t:0x%lx\n",__func__,p);
    spin_lock(&p->lock);
}

void gdev_unlock_nested(gdev_lock_t *p)
{
 //   printk("[%s]: lock_t:0x%lx\n",__func__,p);
    spin_unlock(&p->lock);
}

void gdev_mutex_init(struct gdev_mutex *p)
{
    mutex_init(&p->mutex);
}

void gdev_mutex_lock(struct gdev_mutex *p)
{
    mutex_lock(&p->mutex);
}

void gdev_mutex_unlock(struct gdev_mutex *p)
{
    mutex_unlock(&p->mutex);
}

struct gdev_device* gdev_phys_get(struct gdev_device *gdev)
{
    return gdev->parent? gdev->parent:NULL;
}
