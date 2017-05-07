#ifndef __SCHEDBENCH_H__
#define __SCHEDBENCH_H__

typedef unsigned long ulong_t;

#define TRUE 		1
#define FALSE		0 	

#define MAX_BUF 256

#define RET_SUCCESS 1
#define RET_MISS	0

/* 1000usec = 1ms. */
#define USEC_1MS 1000
/* dummy computation. */
#define DUMMY(reg1, reg2)		\
	do {						\
		int i;					\
		for (i = 0; i < 10; i++)\
			reg2 = reg1 / reg2;	\
	} while(0)

/* macro to loop x times with dummy function. */
#define LOOP(x)								\
	do {									\
		int i;								\
		register int reg1 = 1000, reg2 = 32;\
		for (i = 0; i < x; i++) {			\
			DUMMY(reg1, reg2);				\
		}									\
	} while(0)

/* macro to subsract one timeval from the other. */
#define timeval_sub(a, b, result) 						\
	do {						  						\
		(result)->tv_sec = (a)->tv_sec - (b)->tv_sec;	\
		(result)->tv_usec = (a)->tv_usec - (b)->tv_usec;\
		if ((result)->tv_usec < 0) {					\
			--(result)->tv_sec;							\
			(result)->tv_usec += 1000000;				\
		}												\
	} while (0)

#endif
