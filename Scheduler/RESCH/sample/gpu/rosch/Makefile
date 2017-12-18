TARGET = MyApp
CC= g++
INCDIRS = -I/usr/local/cuda-8.0/targets/x86_64-linux/include
LIBS = -L/usr/lib/resch -lresch_ros_gpu -pthread  /usr/lib/resch/libresch.a -lcuda

NV_ARCH = sm_61

NVCC = /usr/local/cuda-8.0/bin/nvcc -arch $(NV_ARCH) -cubin 
NV_SRC = loop

all: 
	$(NVCC) -o $(NV_SRC)_gpu.cubin $(NV_SRC)_gpu.cu
	$(CC) -o $(TARGET) $(INCDIRS) ros_gpu_sample.cpp $(NV_SRC).cpp $(LIBS)

clean:
	-rm $(TARGET) $(NV_SRC)_gpu.cubin
