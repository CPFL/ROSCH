#ifndef __DEADLINE_H__
#define __DEADLINE_H__

struct sched_attr {
    uint32_t size;
    uint32_t sched_policy;
    uint64_t sched_flags;

    /* SCHED_NORMAL, SCHED_BATCH */
    int32_t sched_nice;
    /* SCHED_FIFO, SCHED_RR */
    uint32_t sched_priority;
    /* SCHED_DEADLINE */
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};
#ifdef __x86_64__
#define __NR_sched_setattr              314
#define __NR_sched_getattr              315
#endif

#ifdef __i386__
#define __NR_sched_setattr              351
#define __NR_sched_getattr              352
#endif

#ifdef __arm__
#define __NR_sched_setattr              380
#define __NR_sched_getattr              381
#endif

#ifndef SCHED_DEADLINE
#define SCHED_DEADLINE          6
#endif

#ifndef SCHED_FLAG_RESET_ON_FORK
#define SCHED_FLAG_RESET_ON_FORK        0x01
#endif
    
    static int
sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags)
{
    return syscall(__NR_sched_setattr, pid, attr, flags);
}

#endif
