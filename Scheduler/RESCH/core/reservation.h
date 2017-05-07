#ifndef __RESCH_RESERVATION_H__
#define __RESCH_RESERVATION_H__

/* APIs. */
int api_reserve_start(int, unsigned long, int);
int api_reserve_stop(int);
int api_reserve_expire(int);
int api_server_run(int);

/* internal functions. */
void reserve_start(resch_task_t *, unsigned long, int);
void reserve_stop(resch_task_t *);
void reserve_replenish(resch_task_t *);
void start_account(resch_task_t *);
void stop_account(resch_task_t *);
void preempt_account(resch_task_t *);
void suspend_account(resch_task_t *);
void resume_account(resch_task_t *);

#endif
