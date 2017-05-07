LIB	= /usr/lib/resch/libresch.a
CFLAGS  = -W -Wall -g -O2 -s -pipe

all: 
	gcc $(CFLAGS) -o rt_task rt_task.c $(LIB)
	gcc $(CFLAGS) -o rt_task_qos rt_task_qos.c $(LIB) 
	gcc $(CFLAGS) -o rt_task_qos_xcpu rt_task_qos_xcpu.c $(LIB) 
#	gcc $(CFLAGS) -o rt_server rt_server.c $(LIB) 
clean:
	rm -f  rt_task rt_task_qos rt_task_qos_xcpu rt_server *~
