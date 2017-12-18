/*
 * tvops.h:	timeval operations.
 */

#ifndef __RESCH_TVOPS_H__
#define __RESCH_TVOPS_H__

#define USEC_1SEC	1000000
#define USEC_1MSEC	1000
#define MSEC_1SEC	1000

/* tvadd: ret = x + y. */
static inline void tvadd(struct timeval *x, 
						 struct timeval *y, 
						 struct timeval *ret)
{
	ret->tv_sec = x->tv_sec + y->tv_sec;
	ret->tv_usec = x->tv_usec + y->tv_usec;
	if (ret->tv_usec >= USEC_1SEC) {
		ret->tv_sec++;
		ret->tv_usec -= USEC_1SEC;
	}
}

/* tvsub: ret = x - y. */
static inline void tvsub(struct timeval *x, 
						 struct timeval *y, 
						 struct timeval *ret)
{
	ret->tv_sec = x->tv_sec - y->tv_sec;
	ret->tv_usec = x->tv_usec - y->tv_usec;
	if (ret->tv_usec < 0) {
		ret->tv_sec--;
		ret->tv_usec += USEC_1SEC;
	}
}

/* tvmul: ret = x * I. */
static inline void tvmul(struct timeval *x, 
						 int I, 
						 struct timeval *ret)
{
	ret->tv_sec = x->tv_sec * I;
	ret->tv_usec = x->tv_usec * I;
	if (ret->tv_usec >= USEC_1SEC) {
		unsigned long carry = ret->tv_usec / USEC_1SEC;
		ret->tv_sec += carry;
		ret->tv_usec -= carry * USEC_1SEC;
	}
}

/* tvdiv: ret = x / I. */
static inline void tvdiv(struct timeval *x, 
						 int I, 
						 struct timeval *ret)
{
	ret->tv_sec = x->tv_sec / I;
	ret->tv_usec = x->tv_usec / I;
}

/* tvge: x >= y. */
static inline int tvge(struct timeval *x, 
					   struct timeval *y)
{
	return x->tv_sec == y->tv_sec ? 
		x->tv_usec >= y->tv_usec :
		x->tv_sec >= y->tv_sec;
}

/* tvge: x <= y. */
static inline int tvle(struct timeval *x, 
					   struct timeval *y)
{
	return x->tv_sec == y->tv_sec ? 
		x->tv_usec <= y->tv_usec :
		x->tv_sec <= y->tv_sec;
}

/* tvms: generate timeval from msecs. */
static inline void tvms(unsigned long ms, struct timeval *tv)
{
	unsigned long carry = ms / MSEC_1SEC;
	tv->tv_sec = carry;
	tv->tv_usec = (ms - carry * MSEC_1SEC) * USEC_1MSEC;
}

/* tvms: generate timeval from usecs. */
static inline void tvus(unsigned long us, struct timeval *tv)
{
	tv->tv_sec = 0;
	tv->tv_usec = us;
}

/* tvclear: clear the value of timeval. */
static inline void tvclear(struct timeval *tv)
{
	tv->tv_sec = tv->tv_usec = 0;
}

/* tvjiffies: generate timeval from jiffies. */
#define tvjiffies(jiffies, tv)	tvms((jiffies), (tv))

#endif
