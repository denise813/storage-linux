/***********************************************************************************
 * 文 件 名   : map.c
 * 负 责 人   : hy
 * 创建日期   : 2018年10月18日
 * 文件描述   :  
 * 版权说明   : Copyright (c) 2008-2019   xx xx xx xx 技术有限公司
 * 其    他   : 
        内核实现了两个映射域,
            一个用于块设备(bdev_map)genhd_device_init函数中进行初始化
        另一个用于字符设备（cdev_map）
 * 修改日志   : 
***********************************************************************************/

// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/drivers/base/map.c
 *
 * (C) Copyright Al Viro 2002,2003
 *
 * NOTE: data structure needs to be changed.  It works, but for large dev_t
 * it will be too slow.  It is isolated, though, so these changes will be
 * local to that file.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kdev_t.h>
#include <linux/kobject.h>
#include <linux/kobj_map.h>

struct kobj_map {
	struct probe {
		struct probe *next; /*指向下一个项的指针*/
		dev_t dev;  /*起始设备编号*/
		unsigned long range;  /*设备编号范围*/
		struct module *owner; /*指向实现这个设备对象模块的指针*/
		kobj_probe_t *get;  /*获取内核对象方法*/
		int (*lock)(dev_t, void *); /*锁定内核对象,以免其被释放的方法*/
		void *data; /*对象私有数据*/
	} *probes[255];
	struct mutex *lock;
};

/*****************************************************************************
 * 函 数 名  : kobj_map
 * 负 责 人  : hy
 * 创建日期  : 2018年10月18日
 * 函数功能  : 添加映射到映射域
 * 输入参数  : struct kobj_map *domain  指向映射域描述符的指针
               dev_t dev                设备编号
               unsigned long range      编号范围
               struct module *module    指向实现了该内核对象的模块的指针
               kobj_probe_t *probe      得内核对象的方法
               int (*lock)(dev_t, void *)                  锁定内核对象
               void *data               设备对象私有数据的指针
 * 输出参数  : 无
 * 返 回 值  : 
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
int kobj_map(struct kobj_map *domain, dev_t dev, unsigned long range,
	     struct module *module, kobj_probe_t *probe,
	     int (*lock)(dev_t, void *), void *data)
{
	unsigned n = MAJOR(dev + range - 1) - MAJOR(dev) + 1;
	unsigned index = MAJOR(dev);
	unsigned i;
	struct probe *p;

	if (n > 255)
		n = 255;

	p = kmalloc_array(n, sizeof(struct probe), GFP_KERNEL);
	if (p == NULL)
		return -ENOMEM;

	for (i = 0; i < n; i++, p++) {
		p->owner = module;
		p->get = probe;
		p->lock = lock;
		p->dev = dev;
		p->range = range;
		p->data = data;
	}
	mutex_lock(domain->lock);
	for (i = 0, p -= n; i < n; i++, p++, index++) {
		struct probe **s = &domain->probes[index % 255];
		while (*s && (*s)->range < range)
			s = &(*s)->next;
		p->next = *s;
		*s = p;
	}
	mutex_unlock(domain->lock);
	return 0;
}

void kobj_unmap(struct kobj_map *domain, dev_t dev, unsigned long range)
{
	unsigned n = MAJOR(dev + range - 1) - MAJOR(dev) + 1;
	unsigned index = MAJOR(dev);
	unsigned i;
	struct probe *found = NULL;

	if (n > 255)
		n = 255;

	mutex_lock(domain->lock);
	for (i = 0; i < n; i++, index++) {
		struct probe **s;
		for (s = &domain->probes[index % 255]; *s; s = &(*s)->next) {
			struct probe *p = *s;
			if (p->dev == dev && p->range == range) {
				*s = p->next;
				if (!found)
					found = p;
				break;
			}
		}
	}
	mutex_unlock(domain->lock);
	kfree(found);
}

/*****************************************************************************
 * 函 数 名  : kobj_lookup
 * 负 责 人  : hy
 * 创建日期  : 2018年10月18日
 * 函数功能  : 映射域中查找具有给定设备号的内核对象
 * 输入参数  : struct kobj_map *domain  向映射域描述符的指针
               dev_t dev                设备编号
               int *index               返回该内核对象在本映射编号范围中的索引
 * 输出参数  : 无
 * 返 回 值  : 返回指向内核对象描述符的指针
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
struct kobject *kobj_lookup(struct kobj_map *domain, dev_t dev, int *index)
{
	struct kobject *kobj;
	struct probe *p;
	unsigned long best = ~0UL;

retry:
	mutex_lock(domain->lock);
	for (p = domain->probes[MAJOR(dev) % 255]; p; p = p->next) {
		struct kobject *(*probe)(dev_t, int *, void *);
		struct module *owner;
		void *data;

		if (p->dev > dev || p->dev + p->range - 1 < dev)
			continue;
		if (p->range - 1 >= best)
			break;
		if (!try_module_get(p->owner))
			continue;
		owner = p->owner;
		data = p->data;
		probe = p->get;
		best = p->range - 1;
		*index = dev - p->dev;
		if (p->lock && p->lock(dev, data) < 0) {
			module_put(owner);
			continue;
		}
		mutex_unlock(domain->lock);
		kobj = probe(dev, index, data);
		/* Currently ->owner protects _only_ ->probe() itself. */
		module_put(owner);
		if (kobj)
			return kobj;
		goto retry;
	}
	mutex_unlock(domain->lock);
	return NULL;
}

/*****************************************************************************
 * 函 数 名  : kobj_map_init
 * 负 责 人  : hy
 * 创建日期  : 2018年10月18日
 * 函数功能  : 初始化映射域
        将分配一个映射域kobj_map结构， 
        以及一个“泛化”probe结构，所有probe链表最终都会链到这个“泛化”项
 * 输入参数  : kobj_probe_t *base_probe   
               struct mutex *lock         
 * 输出参数  : 无
 * 返 回 值  : struct
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
struct kobj_map *kobj_map_init(kobj_probe_t *base_probe, struct mutex *lock)
{
	struct kobj_map *p = kmalloc(sizeof(struct kobj_map), GFP_KERNEL);
	struct probe *base = kzalloc(sizeof(*base), GFP_KERNEL);
	int i;

	if ((p == NULL) || (base == NULL)) {
		kfree(p);
		kfree(base);
		return NULL;
	}

	base->dev = 1;
	base->range = ~0;
	base->get = base_probe;
	for (i = 0; i < 255; i++)
		p->probes[i] = base;
	p->lock = lock;
	return p;
}
