TARGET = fp-pm
TOPDIR = ../../..
COREDIR = $(TOPDIR)/core
INCDIR = $(TOPDIR)/include

# If KERNELRELEASE is define, we have been invoked from the
# kernel build system and can use its languages.
ifneq ($(KERNELRELEASE),)
	obj-m := $(TARGET).o
	EXTRA_CFLAGS := -I$(src)/$(INCDIR)
	KBUILD_EXTRA_SYMBOLS := $(src)/$(COREDIR)/Module.symvers

# Otherwise we were called directly from the command line;
# invoke the kernel build system
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD	:= $(shell pwd)
	EXTRA_CFLAGS := -I$(PWD)/$(INCDIR)
	KBUILD_EXTRA_SYMBOLS := $(PWD)/$(COREDIR)/Module.symvers

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -fr .$(TARGET)* .tmp* *.mod.* *.ko *.o modules.order Module.* *~

install:
	/sbin/insmod $(TARGET).ko

uninstall:
	/sbin/rmmod $(TARGET)
endif
