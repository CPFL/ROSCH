########################################
# makefile for check_ComputeCapability #
########################################

TARGET	= check_GPU
SOURCE	= check_GPU.c

C_COMPILER	= g++

INC_PATH	= -I/usr/local/cuda/include
LIB_PATH	= -L/usr/local/cuda/lib
LIB	= -lcuda

all:
	$(C_COMPILER) $(INC_PATH) $(LIB_PATH) -o $(TARGET) $(SOURCE) $(LIB)

clean:
	rm -f ./*~ ./$(TARGET)
