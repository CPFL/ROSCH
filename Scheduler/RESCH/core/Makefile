TARGET = resch
OBJS_GPU = gpu.o gpu_init.o gpu_sched.o
OBJS = main.o sched.o sched_ros.o preempt_trace.o event.o reservation.o component.o test.o $(OBJS_GPU)
INCDIR = ../include/

# If KERNELRELEASE is define, we have been invoked from the
# kernel build system and can use its languages.
ifneq ($(KERNELRELEASE),)
	obj-m := $(TARGET).o
	$(TARGET)-objs := $(OBJS)
	EXTRA_CFLAGS := -I$(src)/$(INCDIR) -w

# Otherwise we were called directly from the command line;
# invoke the kernel build system
else

	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
	EXTRA_CFLAGS := -I$(PWD)/$(INCDIR) -w

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

$(TARGET).o: $(OBJS)
	$(LD) $(LD_RFLAG) -r -o $@ $(OBJS)

clean:
	rm -fr .tmp* .*.o.cmd *.mod.* *.ko *.o .$(TARGET)* modules.order Module.* *~

.PHONY:install
install:
	@./script/install

.PHONY:uninstall
uninstall:
	@./script/uninstall
endif
