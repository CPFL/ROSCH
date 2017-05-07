#ifndef __RESCH_PREEMPT_TRACE_H__
#define __RESCH_PREEMPT_TRACE_H__

#ifdef RESCH_PREEMPT_TRACE
#ifndef NO_LINUX_LOAD_BALANCE
#error "RESCH_PREEMPT_TRACE requires NO_LINUX_LOAD_BALANCE."
#endif /* NO_LINUX_LOAD_BALANCE */

void preempt(resch_task_t *);
void preempt_self(resch_task_t *);
void preempt_out(resch_task_t *);
void preempt_in(resch_task_t *);
#endif /* RESCH_PREEMPT_TRACE */

#endif /* __RESCH_PREEMPT_TRACE_H__ */
