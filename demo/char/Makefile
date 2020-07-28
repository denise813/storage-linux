# 模块名称
#obj-m := <模块名称>.o
obj-m := democdev.o
#<模块名称>-objs := <目标名称>.o
democdev-objs := simple.o

#内核目录
CPATH=$(PWD)
KDIR =${CPATH}/../..
PWD:=$(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	gcc -o demo demo.c
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf *.o *.ko .depend *.mod.o *.mod.c Module.* modules.* *.mod
	rm -rf demo
