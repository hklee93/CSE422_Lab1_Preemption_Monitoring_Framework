# TODO: Change this to the location of your kernel source code
KERNEL_SOURCE=/tmp/compile/brian.kocoloski/linux_source/linux

INCLUDE_DIR=$(PWD)/../include
EXTRA_CFLAGS += -DMOULE=1 -D__KERNEL__=1 -I$(INCLUDE_DIR)

sched_monitor-objs := $(sched_monitor-y)
obj-m := sched_monitor.o


PHONY: all

all:
	$(MAKE) -C $(KERNEL_SOURCE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SOURCE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$(PWD) clean 
