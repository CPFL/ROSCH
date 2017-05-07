# Makefile
TARGET	= user_test
CC	= gcc
CFLAGS +=-I/usr/local/gdev/include  -I../../include
LIBS = -L/usr/local/gdev/lib64/ -L/usr/lib/resch -lucuda -lgdev -lresch_gpu -lresch -pthread

NV_ARCH = 35 
NVCC = nvcc -arch $(NV_ARCH) -cubin
NV_SRC = loop

all:
	$(NVCC) -o $(NV_SRC)_gpu.cubin $(NV_SRC)_gpu.cu 
	gcc -o $(TARGET) $(CFLAGS) rt_task_gpu.c $(NV_SRC).c  $(LIBS)
clean:
	rm -f $(TARGET)  ./*~ core* $(NV_SRC)_gpu.cubin
