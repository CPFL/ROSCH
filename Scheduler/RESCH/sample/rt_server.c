#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <resch/api.h>
#include "tvops.h"

#define msecs_to_usecs(x) (x) * 1000
#define msecs_to_timeval(ms, tv)					\
	do {											\
		tv.tv_sec = ms / 1000; 						\
		tv.tv_usec = (ms - tv.tv_sec*1000) * 1000; 	\
	} while (0);

int main(int argc, char* argv[])
{
	int i;
	unsigned long prio;
	struct timeval period, wcet, timeout;
	struct timeval tv1, tv2, tv3;

	if (argc != 3) {
		printf("Error: invalid option\n");
	}
	prio = atoi(argv[1]);						/* priority. */
	msecs_to_timeval(atoi(argv[2]), period);	/* period. */
	msecs_to_timeval(1500, wcet);				/* wcet. */
	msecs_to_timeval(2000, timeout);			/* timeout. */

	/* bannar. */
	printf("sample program\n");

	rt_init(); 
	rt_set_priority(prio);
	rt_server_create(&period, &wcet);
	rt_run(&timeout);

	for (i = 0; i < 20; i++) {
		rt_server_run();
		gettimeofday(&tv1, NULL);
		do  {
			gettimeofday(&tv2, NULL);
			/* tv2 - tv1 = tv3 */
			tvsub(&tv2, &tv1, &tv3);
			printf("time %lu:%06lu\n", tv2.tv_sec, tv2.tv_usec);
			fflush(stdout);
		} while (tv3.tv_sec < 1);

		msecs_to_timeval((i % 3) * 500, timeout);
		//msecs_to_timeval(500, timeout);
		rt_sleep(&timeout);
	}
	rt_exit();
	
	return 0;
}
