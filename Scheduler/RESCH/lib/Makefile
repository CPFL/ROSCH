TARGET	= libresch
TARGET_GPU = libresch_gpu
TARGET_ROS_GPU = libresch_ros_gpu
NVRM_OBJS = nvrm/nvrm_ioctl.o nvrm/nvrm_submthd.o nvrm/nvrm_irqctx.o
INCDIR	= ../include/
CFLAGS	= -W -Wall -g -O2 -s -pipe

.PHONY: all
all: $(TARGET).o $(TARGET_GPU).o $(TARGET_ROS_GPU).o $(NVRM_OBJS)
	ar -rcs $(TARGET).a $(TARGET).o
	ar -rcs $(TARGET_GPU).a $(TARGET_GPU).o $(NVRM_OBJS)
	ar -rcs $(TARGET_ROS_GPU).a $(TARGET_ROS_GPU).o
	gcc -shared -Wl,-soname,$(TARGET).so.1 -o ./$(TARGET).so.1.0.0 $(TARGET).o
	gcc -shared -Wl,-soname,$(TARGET_GPU).so.1 -o ./$(TARGET_GPU).so.1.0.0 $(TARGET_GPU).o $(NVRM_OBJS)
	gcc -shared -Wl,-soname,$(TARGET_ROS_GPU).so.1 -o ./$(TARGET_ROS_GPU).so.1.0.0 $(TARGET_ROS_GPU).o

%.o:%.c
	gcc -fPIC -c $^ -o $@ $(CFLAGS) -I$(INCDIR)
	
$(TARGET_ROS_GPU).o:$(TARGET_ROS_GPU).cpp
	gcc -fPIC -c $^ -o $@ $(CFLAGS) -I$(INCDIR)


.PHONY: clean
clean:
	rm -f $(TARGET).o $(TARGET).a $(TARGET).so* *~
	rm -f $(TARGET_GPU).o $(TARGET_GPU).a $(TARGET_GPU).so* *~
	rm -f $(TARGET_ROS_GPU).o $(TARGET_ROS_GPU).a $(TARGET_ROS_GPU).so* *~
	rm -f $(NVRM_OBJS)

.PHONY: install
install:
	@./script/install

.PHONY: uninstall
uninstall:
	@./script/uninstall
