# 模块名称
#obj-m := <模块名称>.o
obj-m := hello_module.o
#<模块名称>-objs := <目标名称>.o
hello-objs := hello_module.o

#内核目录
CPATH=$(PWD)
KDIR =${CPATH}/../..
PWD:=$(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf *.o *.ko .depend *.mod.o *.mod.c Module.* modules.* *.mod
