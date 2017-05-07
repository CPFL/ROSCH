/*
 * Copyright (c) 2014-2015 Yusuke Fujii
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

/* resch */
#include <resch-api.h>
#include <resch-config.h>
#include <resch-core.h>
#include <linux/pci.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <asm/io.h>

/* interrupt  */
#include <linux/signal.h>

/* gpu  */
#include <resch-gpu-core.h>
#include "gpu_proc.h"
#include "nvc0_reg.h"
#include "gpu_io.h"

static const int DEVICE_RESOURCE[4] ={ 60,20,10,10};
// static const int DEVICE_RESOURCE[4] ={ 25,25,25,25};

struct gdev_device gdev_vds[GDEV_DEVICE_MAX_COUNT];
struct gdev_device phys_ds[GDEV_DEVICE_MAX_COUNT];

int gpu_count = 0;
int gpu_vcount = GDEV_DEVICE_MAX_COUNT;

struct resch_irq_desc *resch_desc[GDEV_DEVICE_MAX_COUNT];
struct gdev_sched_entity *sched_entity_ptr[GDEV_CONTEXT_MAX_COUNT];

struct gdev_list list_irq_wakeup_head;
struct gdev_list list_irq_wait_head;
struct gdev_list list_entry_irq_head;

extern spinlock_t reschg_global_spinlock;

extern unsigned long cid_bitmap[100];

void gdev_wakeup_func(unsigned long arg)
{
    struct gdev_device *gdev = (struct gdev_device *)arg; 
    int a = 0;
    /*
       while(!kthread_should_stop()){
       if(!a)
       a=1;
       else if(!kthread_should_stop()){
       if(gdev->sched_com_thread)
       if(!wake_up_process(gdev->sched_com_thread)){
       printk("[%s]:Failed wakeup %d\n",__func__,gdev->id);
       schedule_timeout_interruptible(1);
       while(!wake_up_process(gdev->sched_com_thread)){
       printk("[%s]:Failed wakeup %d\n",__func__,gdev->id);
       schedule_timeout_interruptible(1);
       }
       }
       printk("[%s-%d]:wakeup func end\n",__func__,gdev->id);
       }
       set_current_state(TASK_UNINTERRUPTIBLE);
       schedule();
       }
       */
}

void sched_thread_wakeup_tasklet(unsigned long arg)
{
    struct gdev_device *gdev = (struct gdev_device *)arg;

    if(gdev->sched_com_thread)
	if(!wake_up_process(gdev->sched_com_thread)) {
	    schedule_timeout_interruptible(1);
	    wake_up_process(gdev->sched_com_thread);
	}
}

static inline void print_intr_status(unsigned int intr,unsigned int stat){
#if 0 /*for detailed debugging*/
    int i = 0;	
    while(stat){
	if(stat & nvc0_irq_num[i].num){
	    stat &= ~nvc0_irq_num[i].num;
	    printk("%s ||",nvc0_irq_num[i].dev_name);
	}
	i++;
    }
    printk(")\n");
#endif
}

int engine_gr_intr(void* priv, uint32_t stat)
{
    uint32_t addr, op, cid;
    struct gdev_sched_entity *se = NULL;

    void* task;

    if(stat & 0x3){
	addr = nv_rd32(priv, 0x400704); /* read notify address  */
	op = (addr & 0x00007000) >> 16; /* split addr for to get the interrupt operation */
	cid = nv_rd32(priv, 0x400708) & 0x1f; /* read the cid */
	se = sched_entity_ptr[cid]; 
	if(se){    
	    if(stat & 0x1){
		if( atomic_read(&se->launch_count) ){ /* duplicate check */
		    gdev_list_add_tail(&se->list_wakeup_irq, &list_irq_wakeup_head); // enqueue wakeup_list
		 //   gdev_list_del(&se->list_wait_irq);

		    nv_wr32(priv, 0x400100, 1); /* clear the graph interrupt */
		    nv_wr32(priv, 0x400500, 0x00010001);/* wtf */ 

		    RESCH_G_DPRINT("GDEV_COMPUTE_INTERRUPT! ctx#%d\n", se->ctx);
			
		    if(se->gdev)
			if(se->gdev->sched_com_thread)
			    if(!wake_up_process(se->gdev->sched_com_thread)){
				if(!wake_up_process(se->gdev->sched_com_thread)){
				    //		wake_up_process(se->gdev->wakeup_thread);
				    //    tasklet_schedule(se->gdev->wakeup_tasklet_t);
				    RESCH_G_PRINT("Failed wakeup sched_com_thread\ncall wakeup_tasklet\n");
				}

			    }
		    printk("grintr end\n");

		}else{ /*launch_count*/
		    RESCH_G_DPRINT("INTERRUPT_DUPLICATE!!!!!, ctx#%d\n",cid,atomic_read(&se->launch_count));
		}

		stat = stat & ~0x1;
	    }else if(stat & 0x2){ /* nvrm interrupt was write status is 2 */
		RESCH_G_DPRINT("NVRM_COMPUTE_INTERRUPT! addr:0x%08lx,stat:0x%08lx,cid:0x%08lx\n", addr,stat,cid);
		stat = stat & ~0x2;
	    }
	}else{ /* if se */
	    RESCH_G_DPRINT("Can not found Scheduling Entity........ cid#%d\n",cid);
	}
    }/* stat */

    return stat;
}

static inline void wakeup_list_head_entity(void *priv, struct gdev_list *head){

    struct gdev_sched_entity *se;

    //printk("[%s]",__func__);
    gdev_list_for_each(se, &list_irq_wait_head, list_wait_irq){
	if( atomic_read(&se->launch_count) ){ /* duplicate check */

	    gdev_list_add_tail(&se->list_wakeup_irq, &list_irq_wakeup_head); // enqueue wakeup_list
	    gdev_list_del(&se->list_wait_irq);

	    nv_wr32(priv, 0x400100, 1); /* clear the graph interrupt */
	    nv_wr32(priv, 0x400500, 0x00010001);/* wtf */ 

	    atomic_dec(&se->launch_count);


	    if( gdev_list_container(head->prev) == se)
		if(!wake_up_process(se->gdev->sched_com_thread)){
		    if(!wake_up_process(se->gdev->sched_com_thread)){
			//    tasklet_schedule(se->gdev->wakeup_tasklet_t);
			RESCH_G_PRINT("Failed wakeup sched_com_thread\ncall wakeup_tasklet\n");
		    }
		}
	    break;
	}else{
	    printk("Do not wait se#%d\n",se->ctx);
	}
    }
}


irqreturn_t gsched_intr(int irq, void *arg)
{
    irqreturn_t ret;
    struct resch_irq_desc *__desc = (struct resch_irq_desc*)arg;
    void *priv = __desc->mappings;
    uint32_t intr, stat;;
    struct gdev_sched_entity *se;
    int i=0;

    int addr;

    static int count_b =0, count_a = 0;
    uint32_t hi, lo;
    uint64_t timestamp;
    int pwr_disp, pwr_intr;

    int go_flag = 0;

    nv_wr32(priv, 0x140, 0); /* disable interrupt  */
    if(!gdev_list_head(&list_irq_wait_head) ) /* rtx sched check  */
    {
	count_a=0;
	count_b=0;
	intr = nv_rd32(priv, 0x00100); /* read interrupt status */
//	printk("[%s]:stat:0x%lx\n",__func__, intr);
	if(intr & 0x01000000){ /*pwr*/

	    printk("[%s:%d] pwr intr!\n",__func__,__LINE__);
	}
#if 0
	    pwr_disp = nv_rd32(priv, 0x10a01c);
	    pwr_intr = nv_rd32(priv, 0x10a008) & pwr_disp & ~(pwr_disp >> 16);
	    printk("[%s]:disp:0x%x,intr:0x%x\n",__func__,pwr_disp,pwr_intr);

	    intr = nv_rd32(priv, 0x00100);


	    if(pwr_intr == 0x40){ /* waiting for synchronous reply?  */
		int base = nv_rd32(priv, 0x10a4dc) & 0xffff;
		addr = nv_rd32(priv,0x10a4cc);
		if( addr != nv_rd32(priv, 0x10a4c8 )){
		    do {
			nv_wr32(priv, 0x10a580, 2);
		    } while(nv_rd32(priv, 0x10a580) != 0x2);

		    nv_wr32(priv, 0x10a1c0, 0x02000000 | (((addr & 0x07) <<4) + base));
		    uint32_t process = nv_rd32(priv, 0x10a1c4);
		    uint32_t message = nv_rd32(priv, 0x10a1c4);
		    uint32_t data0 = nv_rd32(priv, 0x10a1c4);
		    uint32_t data1 = nv_rd32(priv, 0x10a1c4);
		    nv_wr32(priv, 0x10a4cc, (addr+1) & 0xf);
		    nv_wr32(priv, 0x10a580, 0x00000000);


		    /* right now there's no other expected responses from the engine,
		     * so assume that any unexpected message is an error.
		     */
		    printk("[%s]:%c%c%c%c 0x%08x 0x%08x 0x%08x 0x%08x\n",
			   __func__, (char)((process & 0x000000ff) >>  0),
			    (char)((process & 0x0000ff00) >>  8),
			    (char)((process & 0x00ff0000) >> 16),
			    (char)((process & 0xff000000) >> 24),
			    process, message, data0, data1);




		}
	    }
	}
#endif
	return __desc->gpu_driver_handler(__desc->irq_num, __desc->dev_id_orig);
    }else
    {
	intr = nv_rd32(priv, 0x00100); /* read interrupt status */
	printk("[%s]:stat:0x%lx>>",__func__, intr);

gr:
	if (intr & NVDEV_ENGINE_GR_INTR)
	{
	    stat = engine_gr_intr(priv, nv_rd32(priv, 0x400100)); /* process pgraph engine interrupt */
	    intr &= ~NVDEV_ENGINE_GR_INTR;
	}
	goto through;
#if 1
	printk("[%s:%d]\n",__func__,__LINE__);
	if(intr & 0x01000000){ /*pwr*/

	    printk("[%s:%d]\n",__func__,__LINE__);
	    pwr_disp = nv_rd32(priv, 0x10a01c);
	    pwr_intr = nv_rd32(priv, 0x10a008) & pwr_disp & ~(pwr_disp >> 16);
	    printk("[%s]:disp:0x%x,intr:0x%x\n",__func__,pwr_disp,pwr_intr);

	    intr = nv_rd32(priv, 0x00100);


	    if(pwr_intr == 0x40){ /* waiting for synchronous reply?  */
		int base = nv_rd32(priv, 0x10a4dc) & 0xffff;
		addr = nv_rd32(priv,0x10a4cc);
		if( addr != nv_rd32(priv, 0x10a4c8 )){
		    printk("aa");
		    do {
			nv_wr32(priv, 0x10a580, 2);
			printk("[%s:loop]\n",__func__);
		    } while(nv_rd32(priv, 0x10a580) != 0x2);

		    nv_wr32(priv, 0x10a1c0, 0x02000000 | (((addr & 0x07) <<4) + base));
		    uint32_t process = nv_rd32(priv, 0x10a1c4);
		    uint32_t message = nv_rd32(priv, 0x10a1c4);
		    uint32_t data0 = nv_rd32(priv, 0x10a1c4);
		    uint32_t data1 = nv_rd32(priv, 0x10a1c4);
		    nv_wr32(priv, 0x10a4cc, (addr+1) & 0xf);
		    nv_wr32(priv, 0x10a580, 0x00000000);


		    /* right now there's no other expected responses from the engine,
		     * so assume that any unexpected message is an error.
		     */
		    printk("%c%c%c%c 0x%08x 0x%08x 0x%08x 0x%08x\n",
			    (char)((process & 0x000000ff) >>  0),
			    (char)((process & 0x0000ff00) >>  8),
			    (char)((process & 0x00ff0000) >> 16),
			    (char)((process & 0xff000000) >> 24),
			    process, message, data0, data1);




		}
		//	nv_wr32(priv, 0x10a004, 0x00000040);
		//goto notcall; 
	    }
	    goto through;

	    printk("[%s:%d]\n",__func__,__LINE__);
	    //	wakeup_list_head_entity(priv, gdev_list_head(&list_irq_wait_head));    
	    //	    }
	    //
    }
#endif
    if(intr & 0x100){
	//wakeup_list_head_entity(priv, gdev_list_head(&list_irq_wait_head));
	goto through;
    }
    if(intr & 0x00100000){/*TIMER*/
	nv_wr32(priv, NV04_PTIMER_INTR_EN_0, 0x0); // disable timer interrupt
	nv_wr32(priv, NV04_PTIMER_INTR_0, 0xffffffff); /* clear flag of timer interrupt*/

	do {
	    hi = nv_rd32(priv, NV04_PTIMER_TIME_1);
	    lo = nv_rd32(priv, NV04_PTIMER_TIME_0);
	} while (hi != nv_rd32(priv, NV04_PTIMER_TIME_1));

	timestamp = (uint64_t) (hi<<32 |  lo + 1000000000ULL); /* next wakeup alarm time  */
	nv_wr32(priv, NV04_PTIMER_ALARM_0, timestamp); // write the next wakeup alarm time
	nv_wr32(priv, NV04_PTIMER_INTR_EN_0, 0x1); // enable timer interrupt
	intr &= ~0x00100000;
	goto notcall;
    }
}
through:
    printk("call driver handler\n");

    ret =  __desc->gpu_driver_handler(__desc->irq_num, __desc->dev_id_orig);

    printk("after call driver handler\n");

    return ret;//ret;

notcall:

    nv_wr08(priv, 0x088068, 0xff);/* msi rearm write */
    nv_wr32(priv, 0x140, 1); /* enable interrupt  */
    return IRQ_NONE;
}

static int gdev_sched_com_thread(void *data)
{
    struct gdev_device *gdev = (struct gdev_device*)data;
    struct gdev_sched_entity *se = NULL;
    struct gdev_device *phys = gdev->parent;
    static int count = 0;

    RESCH_G_PRINT("RESCH_G#%d-%d compute scheduler runnning\n", gdev->parent?gdev->parent->id:0,gdev->id);
    gdev->sched_com_thread = current;

    while (!kthread_should_stop()) {
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();

	/* wakeup sleeping task */
retry_get_list:

	printk("wakeuped com thread: gdev0x%lx\n",gdev);
#if 0
	//	spin_lock( &reschg_global_spinlock );
	se = gdev_list_container(gdev_list_head(&list_irq_wakeup_head));
	if(se){
	    if( se->gdev != gdev){
		gdev_list_for_each(se, &list_irq_wakeup_head, list_wait_irq){
		    if( se && se->gdev == gdev){
			goto wakeup_com;
		    }
		}
		goto no_wakeup_com;
	    }
wakeup_com:
	    if(se){
		RESCH_G_DPRINT("[%s:%d]: compute ctx#%d wakeup \n",__func__,current->pid,se->ctx);
		gdev_list_del(&se->list_wakeup_irq);
		gdev_sched_wakeup(se);
	    }
	//}
	/**/
#endif
no_wakeup_com:
	if (gdev->users)
	{
	    printk("goto next compute\n");
	    gdev_select_next_compute(gdev);
	}
    }
    return 0;
}
static int gdev_sched_mem_thread(void *data)
{
    struct gdev_device *gdev = (struct gdev_device*)data;

    RESCH_G_PRINT("RESCH_G#%d-%d compute scheduler runnning\n", gdev->parent?gdev->parent->id:0,gdev->id);
    gdev->sched_mem_thread = current;

    while (!kthread_should_stop()) {
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
	if (gdev->users)
	    gdev_select_next_memory(gdev);
    }

    return 0;
}

static void gdev_credit_handler(unsigned long data)
{
    struct task_struct *p = (struct task_struct *)data;
    wake_up_process(p);
}


static int gdev_credit_com_thread(void *data)
{
    struct gdev_device *gdev = (struct gdev_device*)data;
    struct gdev_time now, last, elapse, interval;
    struct timer_list timer;
    unsigned long effective_jiffies;
    struct task_struct *p;

    static long stay_count = 0;
    static unsigned long long stay_com;

    RESCH_G_PRINT("RESCH_G#%d compute reserve running\n", gdev->id);

    setup_timer_on_stack(&timer, gdev_credit_handler, (unsigned long)current);

    gdev_time_us(&interval, GDEV_UPDATE_INTERVAL);
    gdev_time_stamp(&last);
    effective_jiffies = jiffies;
    while (!kthread_should_stop()) {

	gdev_replenish_credit_compute(gdev);
	mod_timer(&timer, effective_jiffies + usecs_to_jiffies(gdev->period));
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();

	effective_jiffies = jiffies;

	gdev_lock(&gdev->sched_com_lock);
	gdev_time_stamp(&now);
	gdev_time_sub(&elapse, &now, &last);
	gdev->com_bw_used = gdev->com_time * 100 / gdev_time_to_us(&elapse);
	if (gdev->com_bw_used > 100)
	    gdev->com_bw_used = 100;
	if (gdev_time_ge(&elapse, &interval)) {
	    gdev->com_time = 0;
	    gdev_time_stamp(&last);
	}
	gdev_unlock(&gdev->sched_com_lock);
    }

    local_irq_enable();
    if (timer_pending(&timer)) {
	del_timer_sync(&timer);
    }
    local_irq_disable();
    destroy_timer_on_stack(&timer);

    return 0;
}

static int gdev_credit_mem_thread(void *data)
{
    struct gdev_device *gdev = (struct gdev_device*)data;
    struct gdev_time now, last, elapse, interval;
    struct timer_list timer;
    unsigned long effective_jiffies;

    RESCH_G_PRINT("RESCH_G#%d memory reserve running\n", gdev->id);

    setup_timer_on_stack(&timer, gdev_credit_handler, (unsigned long)current);

    gdev_time_us(&interval, GDEV_UPDATE_INTERVAL);
    gdev_time_stamp(&last);
    effective_jiffies = jiffies;

    while (!kthread_should_stop()) {
	gdev_replenish_credit_memory(gdev);
	mod_timer(&timer, effective_jiffies + usecs_to_jiffies(gdev->period));
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
	effective_jiffies = jiffies;

	gdev_lock(&gdev->sched_mem_lock);
	gdev_time_stamp(&now);
	gdev_time_sub(&elapse, &now, &last);
	gdev->mem_bw_used = gdev->mem_time * 100 / gdev_time_to_us(&elapse);
	if (gdev->mem_bw_used > 100)
	    gdev->mem_bw_used = 100;
	if (gdev_time_ge(&elapse, &interval)) {
	    gdev->mem_time = 0;
	    gdev_time_stamp(&last);
	}
	gdev_unlock(&gdev->sched_mem_lock);
    }

    local_irq_enable();
    if (timer_pending(&timer)) {
	del_timer_sync(&timer);
    }
    local_irq_disable();
    destroy_timer_on_stack(&timer);

    return 0;
}

void gsched_intr_print(void *data){

    int old_cid=0, cid;
    int intr;
    struct resch_irq_desc *__desc = (struct resch_irq_desc*)data;
    void *priv = __desc->mappings;

    while (!kthread_should_stop()) {
	intr = nv_rd32(priv, 0x00100);
	if(intr & NVDEV_ENGINE_GR_INTR){
	    cid = nv_rd32(priv, 0x400708);
	    if(old_cid != cid){
		old_cid = cid;
	    }

	}
	yield();
    }
}

#define ENABLE_CREDIT_THREAD
void gsched_create_scheduler(struct gdev_device *gdev)
{
    struct task_struct *sched_com, *sched_mem;
    struct task_struct *credit_com, *credit_mem;
    char name[64];
    struct gdev_device *phys = gdev->parent;
    struct sched_param sp = {.sched_priority = MAX_RT_PRIO -1 };

    struct task_struct *wake;


    /* create compute and memory scheduler threads. */
    sprintf(name, "gschedc%d", gdev->id);
    sched_com = kthread_create(gdev_sched_com_thread, (void*)gdev, name);
    if (sched_com) {
	sched_setscheduler(sched_com, SCHED_FIFO, &sp);
	wake_up_process(sched_com);
	gdev->sched_com_thread = sched_com;
    }
#ifdef ENABLE_MEM_SCHED
    sprintf(name, "gschedm%d", gdev->id);
    sched_mem = kthread_create(gdev_sched_mem_thread, (void*)gdev, name);
    if (sched_mem) {
	sched_setscheduler(sched_mem, SCHED_FIFO, &sp);
	wake_up_process(sched_mem);
	gdev->sched_mem_thread = sched_mem;
    }
#endif

    /* create compute and memory credit replenishment threads. */
    sprintf(name, "gcreditc%d", gdev->id);
    credit_com = kthread_create(gdev_credit_com_thread, (void*)gdev, name);
    if (credit_com) {
	sched_setscheduler(credit_com, SCHED_FIFO, &sp);
	wake_up_process(credit_com);
	gdev->credit_com_thread = credit_com;
    }

#ifdef ENABLE_MEM_SCHED
    sprintf(name, "gcreditm%d", gdev->id);
    credit_mem = kthread_create(gdev_credit_mem_thread, (void*)gdev, name);
    if (credit_mem) {
	sched_setscheduler(credit_mem, SCHED_FIFO, &sp);
	wake_up_process(credit_mem);
	gdev->credit_mem_thread = credit_mem;
    }
#endif
/*
    wake = kthread_create(gdev_wakeup_func, (void*)gdev,"gwakeup");
    if(wake){
	sched_setscheduler(wake, SCHED_FIFO, &sp);
	wake_up_process(wake);
	gdev->wakeup_thread = wake;
    }
  */
    /* set up virtual GPU schedulers, if required. */
    if (phys) {
	gdev_lock(&phys->sched_com_lock);
	gdev_list_init(&gdev->list_entry_com, (void*)gdev);
	gdev_list_add(&gdev->list_entry_com, &phys->sched_com_list);
	gdev_unlock(&phys->sched_com_lock);
	gdev_replenish_credit_compute(gdev);

	gdev_lock(&phys->sched_mem_lock);
	gdev_list_init(&gdev->list_entry_mem, (void*)gdev);
	gdev_list_add(&gdev->list_entry_mem, &phys->sched_mem_list);
	gdev_unlock(&phys->sched_mem_lock);
	gdev_replenish_credit_memory(gdev);
    }
    return 0;
}

void gsched_destroy_scheduler(struct gdev_device *gdev)
{
#ifdef ENABLE_MEM_SCHED
    if (gdev->credit_mem_thread) {
	kthread_stop(gdev->credit_mem_thread);
	gdev->credit_mem_thread = NULL;
    }
#endif

#ifdef ENABLE_CREDIT_THREAD
    if (gdev->credit_com_thread) {
	kthread_stop(gdev->credit_com_thread);
	gdev->credit_com_thread = NULL;
    }
#endif
#ifdef ENABLE_MEM_SCHED
    if (gdev->sched_mem_thread) {
	kthread_stop(gdev->sched_mem_thread);
	gdev->sched_mem_thread = NULL;
    }
#endif
    if (gdev->sched_com_thread) {
	kthread_stop(gdev->sched_com_thread);
	gdev->sched_com_thread = NULL;
    }
    schedule_timeout_uninterruptible(usecs_to_jiffies(gdev->period));
}


void gpu_device_init(struct gdev_device *dev, int id)
{
    dev->id = id;
    dev->users = 0;
    dev->accessed = 0;
    dev->blocked = 0;
    dev->com_bw = 0;
    dev->mem_bw = 0;
    dev->period = 0;
    dev->com_time = 0;
    dev->mem_time = 0;
    dev->sched_com_thread = NULL;
    dev->sched_mem_thread = NULL;
    dev->credit_com_thread = NULL;
    dev->credit_mem_thread = NULL;
    dev->current_com = NULL;
    dev->current_mem = NULL;
    dev->parent = NULL;
    dev->priv = NULL;
    gdev_time_us(&dev->credit_com, 0);
    gdev_time_us(&dev->credit_mem, 0);
    gdev_list_init(&dev->sched_com_list, NULL);
    gdev_list_init(&dev->sched_mem_list, NULL);
    gdev_list_init(&dev->vas_list, NULL);
    gdev_list_init(&dev->shm_list, NULL);
    gdev_lock_init(&dev->sched_com_lock);
    gdev_lock_init(&dev->sched_mem_lock);
    gdev_lock_init(&dev->vas_lock);
    gdev_lock_init(&dev->global_lock);
    gdev_mutex_init(&dev->shm_mutex);

    /* ialized tasklet  */
    //dev->wakeup_tasklet_t = (struct tasklet_struct*)kmalloc(sizeof(struct tasklet_struct), GFP_KERNEL);
    //tasklet_init(dev->wakeup_tasklet_t, sched_thread_wakeup_tasklet, (unsigned long)dev);
}


int gpu_virtual_device_init(struct gdev_device *dev, int id, uint32_t weight, struct gdev_device *phys)
{
    gpu_device_init(dev, id);
    dev->period = GDEV_PERIOD_DEFAULT;
    dev->parent = phys;
    dev->com_bw = weight;
    dev->mem_bw = weight;
    return 0;
}

int gsched_irq_intercept_init(struct resch_irq_desc *desc)
{
    struct pci_dev *__dev = NULL;
    struct resch_irq_desc *__desc = NULL;

    /* registration of hook interrupt handler */
    request_irq(desc->irq_num, gsched_intr/*change*/, IRQF_SHARED, "resch", desc);
    /* release interrupt handler of the nouveau */
    free_irq(desc->irq_num, desc->dev_id_orig);

}

void nouveau_intr_exit(struct resch_irq_desc **desc)
{
    int i;
    struct resch_irq_desc *__desc;
    for (i = 0; i< gpu_count; i++){
	__desc = desc[i];
	if(__desc){
	    request_irq(__desc->irq_num, __desc->gpu_driver_handler, IRQF_SHARED, __desc->gpu_driver_name, __desc->dev_id_orig);
	    free_irq(__desc->irq_num, __desc);
	}
    }
}

int gsched_pci_init(struct resch_irq_desc **desc_top)
{
    struct pci_dev *__dev = NULL;
    struct resch_irq_desc *__rdesc = NULL;
    struct irqaction *__act = NULL;
    int __gpu_count = 0;

    while(__dev = pci_get_class(PCI_CLASS_DISPLAY_VGA << 8, __dev))
    {
	__act = irq_to_desc(__dev->irq)->action;

	__rdesc = (struct resch_irq_desc*)kmalloc(sizeof(struct resch_irq_desc), GFP_KERNEL);
	__rdesc->dev = __dev;
	__rdesc->irq_num = __dev->irq;
	__rdesc->gpu_driver_name = __act->name;
	__rdesc->dev_id_orig = __act->dev_id;
	__rdesc->gpu_driver_handler = __act->handler;
	__rdesc->sched_flag = 0;
	__rdesc->mappings = ioremap(pci_resource_start(__dev, MMIO_BAR0), pci_resource_len(__dev, MMIO_BAR0)); 
	printk("%d devices remapped [0x%lx-0x%lx].sizeof:0x%lx\n",__dev->irq, pci_resource_start(__dev, MMIO_BAR0),pci_resource_end(__dev,MMIO_BAR0), pci_resource_len(__dev,MMIO_BAR0));

	gsched_irq_intercept_init(__rdesc);
	gdev_lock_init(&__rdesc->release_lock);
	desc_top[__gpu_count] = __rdesc;

	__gpu_count++;
	if(__gpu_count >= NR_GPU_CARD_LIMIT)
	    break;
    }
//    atomic_set(&resch_desc[0]->intr_flag,0);

    return __gpu_count;
}

void gsched_init(void)
{
    unsigned long rstart,rend,rflags;
    int vendor_id, device_id, class, sub_vendor_id, sub_device_id, irq;
    int gpu_device_num;
    int i;

    /* look at pci devices */

    gpu_count = gsched_pci_init(resch_desc);

    if(!gpu_count)
	return -ENODEV;

    /* create physical device */
    for (i = 0; i < gpu_count; i++)
	gpu_device_init(&phys_ds[i], i);

    RESCH_G_PRINT("Found %d physical device(s).\n-Initialize device structure.....", gpu_count);


#ifdef ALLOC_VGPU_PER_ONETASK
    gpu_vcount = 100;
#endif

    /* create virtual device  */
    /* create scheduler thread */
#if 1
    for (i = 0; i< gpu_vcount; i++){
#ifndef ALLOC_VGPU_PER_ONETASK
	//gpu_virtual_device_init(&gdev_vds[i], i, 100/gpu_vcount, &phys_ds[i/ (gpu_vcount/gpu_count) ]);
	gpu_virtual_device_init(&gdev_vds[i], i, DEVICE_RESOURCE[i], &phys_ds[0]);
	gsched_create_scheduler(&gdev_vds[i]);
#else 
	gpu_virtual_device_init(&gdev_vds[i], i, 100, NULL);
#endif
    }
#endif

    for(i=0;i<GDEV_CONTEXT_MAX_COUNT;i++)
	sched_entity_ptr[i]=NULL;

    RESCH_G_PRINT("Configured %d virtual device(s). \n", gpu_vcount);
    RESCH_G_PRINT("Each physical device(s) have %d virtual device(s).\n", gpu_vcount/gpu_count);

    /* create /proc entries */
    gdev_proc_create();

    for (i = 0; i< gpu_vcount; i++)
	gdev_proc_minor_create(i);

    for(i=0;i<100;i++)
	cid_bitmap[i]=0;

    /* for irq list */
    gdev_list_init(&list_irq_wakeup_head, NULL);
    gdev_list_init(&list_irq_wait_head, NULL);
}

void gsched_exit(void)
{
    int i;
    /*minmor*/
#ifndef ALLOC_VGPU_PER_ONETASK
    for (i = 0; i< gpu_vcount; i++){
	gsched_destroy_scheduler(&gdev_vds[i]);
    }
#endif
    i = 0;
    nouveau_intr_exit(resch_desc);
    /*unsetnotify*/
    gdev_proc_delete();
}
