/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LINUX_MEMBLOCK_H
#define _LINUX_MEMBLOCK_H
#ifdef __KERNEL__

/*
 * Logical memory blocks.
 *
 * Copyright (C) 2001 Peter Bergner, IBM Corp.
 */

#include <linux/init.h>
#include <linux/mm.h>
#include <asm/dma.h>

extern unsigned long max_low_pfn;
extern unsigned long min_low_pfn;

/*
 * highest page
 */
extern unsigned long max_pfn;
/*
 * highest possible page
 */
extern unsigned long long max_possible_pfn;

/**
 * enum memblock_flags - definition of memory region attributes
 * @MEMBLOCK_NONE: no special request
 * @MEMBLOCK_HOTPLUG: hotpluggable region
 * @MEMBLOCK_MIRROR: mirrored region
 * @MEMBLOCK_NOMAP: don't add to kernel direct mapping
 */
enum memblock_flags {
/** comment by hy 2018-12-31
 * # 无特殊要求
 */
	MEMBLOCK_NONE		= 0x0,	/* No special request */
/** comment by hy 2018-12-31
 * # 可热拔插区域
 */
	MEMBLOCK_HOTPLUG	= 0x1,	/* hotpluggable region */
/** comment by hy 2018-12-31
 * # 镜像区域,将内存复制分别放在主存和镜像内存中
 */
	MEMBLOCK_MIRROR		= 0x2,	/* mirrored region */
/** comment by hy 2018-12-31
 * # 不添加到内核直接映射,即线性映射区
 */
	MEMBLOCK_NOMAP		= 0x4,	/* don't add to kernel direct mapping */
};

/**
 * struct memblock_region - represents a memory region
 * @base: base address of the region
 * @size: size of the region
 * @flags: memory region attributes
 * @nid: NUMA node id
 */
struct memblock_region {
/** comment by hy 2018-12-31
 * # 起始物理地址
 */
	phys_addr_t base;
/** comment by hy 2018-12-31
 * # 长度
 */
	phys_addr_t size;
/** comment by hy 2018-12-31
 * # 标志
 */
	enum memblock_flags flags;
/** comment by hy 2018-12-31
 * # 节点编号
 */
#ifdef CONFIG_NEED_MULTIPLE_NODES
	int nid;
#endif
};

/**
 * struct memblock_type - collection of memory regions of certain type
 * @cnt: number of regions
 * @max: size of the allocated array
 * @total_size: size of all regions
 * @regions: array of regions
 * @name: the memory type symbolic name
 */
struct memblock_type {
/** comment by hy 2018-12-31
 * # 区域数量
 */
	unsigned long cnt;
/** comment by hy 2018-12-31
 * # 已分配数组的大小
 */
	unsigned long max;
/** comment by hy 2018-12-31
 * # 所属区域所有内存块的总长度
 */
	phys_addr_t total_size;
/** comment by hy 2018-12-31
 * # 指向内存区域数组
 */
	struct memblock_region *regions;
/** comment by hy 2018-12-31
 * # 内块类型的名称
 */
	char *name;
};

/**
 * struct memblock - memblock allocator metadata
 * @bottom_up: is bottom up direction?
 * @current_limit: physical address of the current allocation limit
 * @memory: usable memory regions
 * @reserved: reserved memory regions
 * @physmem: all physical memory
 */
struct memblock {
/** comment by hy 2019-12-31
 * # 是从下向上的方向
 */
	bool bottom_up;  /* is bottom up direction? */
/** comment by hy 2019-12-31
 * # 可分配内存的最大物理地址
 */
	phys_addr_t current_limit;
/** comment by hy 2019-12-31
 * # 内存类型
 */
	struct memblock_type memory;
/** comment by hy 2019-12-31
 * # 预留类型
 */
	struct memblock_type reserved;
#ifdef CONFIG_HAVE_MEMBLOCK_PHYS_MAP
/** comment by hy 2019-12-31
 * # 物理类型
 */
	struct memblock_type physmem;
#endif
};

extern struct memblock memblock;
extern int memblock_debug;

#ifndef CONFIG_ARCH_KEEP_MEMBLOCK
#define __init_memblock __meminit
#define __initdata_memblock __meminitdata
void memblock_discard(void);
#else
#define __init_memblock
#define __initdata_memblock
static inline void memblock_discard(void) {}
#endif

#define memblock_dbg(fmt, ...) \
	if (memblock_debug) printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)

phys_addr_t memblock_find_in_range(phys_addr_t start, phys_addr_t end,
				   phys_addr_t size, phys_addr_t align);
void memblock_allow_resize(void);
int memblock_add_node(phys_addr_t base, phys_addr_t size, int nid);
int memblock_add(phys_addr_t base, phys_addr_t size);
int memblock_remove(phys_addr_t base, phys_addr_t size);
int memblock_free(phys_addr_t base, phys_addr_t size);
int memblock_reserve(phys_addr_t base, phys_addr_t size);
#ifdef CONFIG_HAVE_MEMBLOCK_PHYS_MAP
int memblock_physmem_add(phys_addr_t base, phys_addr_t size);
#endif
void memblock_trim_memory(phys_addr_t align);
bool memblock_overlaps_region(struct memblock_type *type,
			      phys_addr_t base, phys_addr_t size);
int memblock_mark_hotplug(phys_addr_t base, phys_addr_t size);
int memblock_clear_hotplug(phys_addr_t base, phys_addr_t size);
int memblock_mark_mirror(phys_addr_t base, phys_addr_t size);
int memblock_mark_nomap(phys_addr_t base, phys_addr_t size);
int memblock_clear_nomap(phys_addr_t base, phys_addr_t size);

unsigned long memblock_free_all(void);
void reset_node_managed_pages(pg_data_t *pgdat);
void reset_all_zones_managed_pages(void);

/* Low level functions */
void __next_mem_range(u64 *idx, int nid, enum memblock_flags flags,
		      struct memblock_type *type_a,
		      struct memblock_type *type_b, phys_addr_t *out_start,
		      phys_addr_t *out_end, int *out_nid);

void __next_mem_range_rev(u64 *idx, int nid, enum memblock_flags flags,
			  struct memblock_type *type_a,
			  struct memblock_type *type_b, phys_addr_t *out_start,
			  phys_addr_t *out_end, int *out_nid);

void __next_reserved_mem_region(u64 *idx, phys_addr_t *out_start,
				phys_addr_t *out_end);

void __memblock_free_late(phys_addr_t base, phys_addr_t size);

/**
 * for_each_mem_range - iterate through memblock areas from type_a and not
 * included in type_b. Or just type_a if type_b is NULL.
 * @i: u64 used as loop variable
 * @type_a: ptr to memblock_type to iterate
 * @type_b: ptr to memblock_type which excludes from the iteration
 * @nid: node selector, %NUMA_NO_NODE for all nodes
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 * @p_nid: ptr to int for nid of the range, can be %NULL
 */
#define for_each_mem_range(i, type_a, type_b, nid, flags,		\
			   p_start, p_end, p_nid)			\
	for (i = 0, __next_mem_range(&i, nid, flags, type_a, type_b,	\
				     p_start, p_end, p_nid);		\
	     i != (u64)ULLONG_MAX;					\
	     __next_mem_range(&i, nid, flags, type_a, type_b,		\
			      p_start, p_end, p_nid))

/**
 * for_each_mem_range_rev - reverse iterate through memblock areas from
 * type_a and not included in type_b. Or just type_a if type_b is NULL.
 * @i: u64 used as loop variable
 * @type_a: ptr to memblock_type to iterate
 * @type_b: ptr to memblock_type which excludes from the iteration
 * @nid: node selector, %NUMA_NO_NODE for all nodes
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 * @p_nid: ptr to int for nid of the range, can be %NULL
 */
#define for_each_mem_range_rev(i, type_a, type_b, nid, flags,		\
			       p_start, p_end, p_nid)			\
	for (i = (u64)ULLONG_MAX,					\
		     __next_mem_range_rev(&i, nid, flags, type_a, type_b,\
					  p_start, p_end, p_nid);	\
	     i != (u64)ULLONG_MAX;					\
	     __next_mem_range_rev(&i, nid, flags, type_a, type_b,	\
				  p_start, p_end, p_nid))

/**
 * for_each_reserved_mem_region - iterate over all reserved memblock areas
 * @i: u64 used as loop variable
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 *
 * Walks over reserved areas of memblock. Available as soon as memblock
 * is initialized.
 */
#define for_each_reserved_mem_region(i, p_start, p_end)			\
	for (i = 0UL, __next_reserved_mem_region(&i, p_start, p_end);	\
	     i != (u64)ULLONG_MAX;					\
	     __next_reserved_mem_region(&i, p_start, p_end))

static inline bool memblock_is_hotpluggable(struct memblock_region *m)
{
	return m->flags & MEMBLOCK_HOTPLUG;
}

static inline bool memblock_is_mirror(struct memblock_region *m)
{
	return m->flags & MEMBLOCK_MIRROR;
}

static inline bool memblock_is_nomap(struct memblock_region *m)
{
	return m->flags & MEMBLOCK_NOMAP;
}

int memblock_search_pfn_nid(unsigned long pfn, unsigned long *start_pfn,
			    unsigned long  *end_pfn);
void __next_mem_pfn_range(int *idx, int nid, unsigned long *out_start_pfn,
			  unsigned long *out_end_pfn, int *out_nid);

/**
 * for_each_mem_pfn_range - early memory pfn range iterator
 * @i: an integer used as loop variable
 * @nid: node selector, %MAX_NUMNODES for all nodes
 * @p_start: ptr to ulong for start pfn of the range, can be %NULL
 * @p_end: ptr to ulong for end pfn of the range, can be %NULL
 * @p_nid: ptr to int for nid of the range, can be %NULL
 *
 * Walks over configured memory ranges.
 */
#define for_each_mem_pfn_range(i, nid, p_start, p_end, p_nid)		\
	for (i = -1, __next_mem_pfn_range(&i, nid, p_start, p_end, p_nid); \
	     i >= 0; __next_mem_pfn_range(&i, nid, p_start, p_end, p_nid))

#ifdef CONFIG_DEFERRED_STRUCT_PAGE_INIT
void __next_mem_pfn_range_in_zone(u64 *idx, struct zone *zone,
				  unsigned long *out_spfn,
				  unsigned long *out_epfn);
/**
 * for_each_free_mem_range_in_zone - iterate through zone specific free
 * memblock areas
 * @i: u64 used as loop variable
 * @zone: zone in which all of the memory blocks reside
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 *
 * Walks over free (memory && !reserved) areas of memblock in a specific
 * zone. Available once memblock and an empty zone is initialized. The main
 * assumption is that the zone start, end, and pgdat have been associated.
 * This way we can use the zone to determine NUMA node, and if a given part
 * of the memblock is valid for the zone.
 */
#define for_each_free_mem_pfn_range_in_zone(i, zone, p_start, p_end)	\
	for (i = 0,							\
	     __next_mem_pfn_range_in_zone(&i, zone, p_start, p_end);	\
	     i != U64_MAX;					\
	     __next_mem_pfn_range_in_zone(&i, zone, p_start, p_end))

/**
 * for_each_free_mem_range_in_zone_from - iterate through zone specific
 * free memblock areas from a given point
 * @i: u64 used as loop variable
 * @zone: zone in which all of the memory blocks reside
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 *
 * Walks over free (memory && !reserved) areas of memblock in a specific
 * zone, continuing from current position. Available as soon as memblock is
 * initialized.
 */
#define for_each_free_mem_pfn_range_in_zone_from(i, zone, p_start, p_end) \
	for (; i != U64_MAX;					  \
	     __next_mem_pfn_range_in_zone(&i, zone, p_start, p_end))

int __init deferred_page_init_max_threads(const struct cpumask *node_cpumask);

#endif /* CONFIG_DEFERRED_STRUCT_PAGE_INIT */

/**
 * for_each_free_mem_range - iterate through free memblock areas
 * @i: u64 used as loop variable
 * @nid: node selector, %NUMA_NO_NODE for all nodes
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 * @p_nid: ptr to int for nid of the range, can be %NULL
 *
 * Walks over free (memory && !reserved) areas of memblock.  Available as
 * soon as memblock is initialized.
 */
#define for_each_free_mem_range(i, nid, flags, p_start, p_end, p_nid)	\
	for_each_mem_range(i, &memblock.memory, &memblock.reserved,	\
			   nid, flags, p_start, p_end, p_nid)

/**
 * for_each_free_mem_range_reverse - rev-iterate through free memblock areas
 * @i: u64 used as loop variable
 * @nid: node selector, %NUMA_NO_NODE for all nodes
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 * @p_nid: ptr to int for nid of the range, can be %NULL
 *
 * Walks over free (memory && !reserved) areas of memblock in reverse
 * order.  Available as soon as memblock is initialized.
 */
#define for_each_free_mem_range_reverse(i, nid, flags, p_start, p_end,	\
					p_nid)				\
	for_each_mem_range_rev(i, &memblock.memory, &memblock.reserved,	\
			       nid, flags, p_start, p_end, p_nid)

int memblock_set_node(phys_addr_t base, phys_addr_t size,
		      struct memblock_type *type, int nid);

#ifdef CONFIG_NEED_MULTIPLE_NODES
static inline void memblock_set_region_node(struct memblock_region *r, int nid)
{
	r->nid = nid;
}

static inline int memblock_get_region_node(const struct memblock_region *r)
{
	return r->nid;
}
#else
static inline void memblock_set_region_node(struct memblock_region *r, int nid)
{
}

static inline int memblock_get_region_node(const struct memblock_region *r)
{
	return 0;
}
#endif /* CONFIG_NEED_MULTIPLE_NODES */

/* Flags for memblock allocation APIs */
#define MEMBLOCK_ALLOC_ANYWHERE	(~(phys_addr_t)0)
#define MEMBLOCK_ALLOC_ACCESSIBLE	0
#define MEMBLOCK_ALLOC_KASAN		1

/* We are using top down, so it is safe to use 0 here */
#define MEMBLOCK_LOW_LIMIT 0

#ifndef ARCH_LOW_ADDRESS_LIMIT
#define ARCH_LOW_ADDRESS_LIMIT  0xffffffffUL
#endif

phys_addr_t memblock_phys_alloc_range(phys_addr_t size, phys_addr_t align,
				      phys_addr_t start, phys_addr_t end);
phys_addr_t memblock_alloc_range_nid(phys_addr_t size,
				      phys_addr_t align, phys_addr_t start,
				      phys_addr_t end, int nid, bool exact_nid);
phys_addr_t memblock_phys_alloc_try_nid(phys_addr_t size, phys_addr_t align, int nid);

static inline phys_addr_t memblock_phys_alloc(phys_addr_t size,
					      phys_addr_t align)
{
	return memblock_phys_alloc_range(size, align, 0,
					 MEMBLOCK_ALLOC_ACCESSIBLE);
}

void *memblock_alloc_exact_nid_raw(phys_addr_t size, phys_addr_t align,
				 phys_addr_t min_addr, phys_addr_t max_addr,
				 int nid);
void *memblock_alloc_try_nid_raw(phys_addr_t size, phys_addr_t align,
				 phys_addr_t min_addr, phys_addr_t max_addr,
				 int nid);
void *memblock_alloc_try_nid(phys_addr_t size, phys_addr_t align,
			     phys_addr_t min_addr, phys_addr_t max_addr,
			     int nid);

/*****************************************************************************
 * 函 数 名  : memblock_alloc
 * 负 责 人  : hy
 * 创建日期  : 2018年12月31日
 * 函数功能  : 分配内存
 * 输入参数  : phys_addr_t size    
               phys_addr_t align   
 * 输出参数  : 无
 * 返 回 值  : static
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
static inline void * __init memblock_alloc(phys_addr_t size,  phys_addr_t align)
{
/** comment by hy 2018-12-31
 * # 找到没有分配的内存块区域
 */
	return memblock_alloc_try_nid(size, align, MEMBLOCK_LOW_LIMIT,
				      MEMBLOCK_ALLOC_ACCESSIBLE, NUMA_NO_NODE);
}

static inline void * __init memblock_alloc_raw(phys_addr_t size,
					       phys_addr_t align)
{
	return memblock_alloc_try_nid_raw(size, align, MEMBLOCK_LOW_LIMIT,
					  MEMBLOCK_ALLOC_ACCESSIBLE,
					  NUMA_NO_NODE);
}

static inline void * __init memblock_alloc_from(phys_addr_t size,
						phys_addr_t align,
						phys_addr_t min_addr)
{
	return memblock_alloc_try_nid(size, align, min_addr,
				      MEMBLOCK_ALLOC_ACCESSIBLE, NUMA_NO_NODE);
}

static inline void * __init memblock_alloc_low(phys_addr_t size,
					       phys_addr_t align)
{
	return memblock_alloc_try_nid(size, align, MEMBLOCK_LOW_LIMIT,
				      ARCH_LOW_ADDRESS_LIMIT, NUMA_NO_NODE);
}

static inline void * __init memblock_alloc_node(phys_addr_t size,
						phys_addr_t align, int nid)
{
	return memblock_alloc_try_nid(size, align, MEMBLOCK_LOW_LIMIT,
				      MEMBLOCK_ALLOC_ACCESSIBLE, nid);
}

static inline void __init memblock_free_early(phys_addr_t base,
					      phys_addr_t size)
{
	memblock_free(base, size);
}

static inline void __init memblock_free_early_nid(phys_addr_t base,
						  phys_addr_t size, int nid)
{
	memblock_free(base, size);
}

static inline void __init memblock_free_late(phys_addr_t base, phys_addr_t size)
{
	__memblock_free_late(base, size);
}

/*
 * Set the allocation direction to bottom-up or top-down.
 */
static inline void __init memblock_set_bottom_up(bool enable)
{
	memblock.bottom_up = enable;
}

/*
 * Check if the allocation direction is bottom-up or not.
 * if this is true, that said, memblock will allocate memory
 * in bottom-up direction.
 */
static inline bool memblock_bottom_up(void)
{
	return memblock.bottom_up;
}

phys_addr_t memblock_phys_mem_size(void);
phys_addr_t memblock_reserved_size(void);
phys_addr_t memblock_mem_size(unsigned long limit_pfn);
phys_addr_t memblock_start_of_DRAM(void);
phys_addr_t memblock_end_of_DRAM(void);
void memblock_enforce_memory_limit(phys_addr_t memory_limit);
void memblock_cap_memory_range(phys_addr_t base, phys_addr_t size);
void memblock_mem_limit_remove_map(phys_addr_t limit);
bool memblock_is_memory(phys_addr_t addr);
bool memblock_is_map_memory(phys_addr_t addr);
bool memblock_is_region_memory(phys_addr_t base, phys_addr_t size);
bool memblock_is_reserved(phys_addr_t addr);
bool memblock_is_region_reserved(phys_addr_t base, phys_addr_t size);

extern void __memblock_dump_all(void);

static inline void memblock_dump_all(void)
{
	if (memblock_debug)
		__memblock_dump_all();
}

/**
 * memblock_set_current_limit - Set the current allocation limit to allow
 *                         limiting allocations to what is currently
 *                         accessible during boot
 * @limit: New limit value (physical address)
 */
void memblock_set_current_limit(phys_addr_t limit);


phys_addr_t memblock_get_current_limit(void);

/*
 * pfn conversion functions
 *
 * While the memory MEMBLOCKs should always be page aligned, the reserved
 * MEMBLOCKs may not be. This accessor attempt to provide a very clear
 * idea of what they return for such non aligned MEMBLOCKs.
 */

/**
 * memblock_region_memory_base_pfn - get the lowest pfn of the memory region
 * @reg: memblock_region structure
 *
 * Return: the lowest pfn intersecting with the memory region
 */
static inline unsigned long memblock_region_memory_base_pfn(const struct memblock_region *reg)
{
	return PFN_UP(reg->base);
}

/**
 * memblock_region_memory_end_pfn - get the end pfn of the memory region
 * @reg: memblock_region structure
 *
 * Return: the end_pfn of the reserved region
 */
static inline unsigned long memblock_region_memory_end_pfn(const struct memblock_region *reg)
{
	return PFN_DOWN(reg->base + reg->size);
}

/**
 * memblock_region_reserved_base_pfn - get the lowest pfn of the reserved region
 * @reg: memblock_region structure
 *
 * Return: the lowest pfn intersecting with the reserved region
 */
static inline unsigned long memblock_region_reserved_base_pfn(const struct memblock_region *reg)
{
	return PFN_DOWN(reg->base);
}

/**
 * memblock_region_reserved_end_pfn - get the end pfn of the reserved region
 * @reg: memblock_region structure
 *
 * Return: the end_pfn of the reserved region
 */
static inline unsigned long memblock_region_reserved_end_pfn(const struct memblock_region *reg)
{
	return PFN_UP(reg->base + reg->size);
}

#define for_each_memblock(memblock_type, region)					\
	for (region = memblock.memblock_type.regions;					\
	     region < (memblock.memblock_type.regions + memblock.memblock_type.cnt);	\
	     region++)

#define for_each_memblock_type(i, memblock_type, rgn)			\
	for (i = 0, rgn = &memblock_type->regions[0];			\
	     i < memblock_type->cnt;					\
	     i++, rgn = &memblock_type->regions[i])

extern void *alloc_large_system_hash(const char *tablename,
				     unsigned long bucketsize,
				     unsigned long numentries,
				     int scale,
				     int flags,
				     unsigned int *_hash_shift,
				     unsigned int *_hash_mask,
				     unsigned long low_limit,
				     unsigned long high_limit);

#define HASH_EARLY	0x00000001	/* Allocating during early boot? */
#define HASH_SMALL	0x00000002	/* sub-page allocation allowed, min
					 * shift passed via *_hash_shift */
#define HASH_ZERO	0x00000004	/* Zero allocated hash table */

/* Only NUMA needs hash distribution. 64bit NUMA architectures have
 * sufficient vmalloc space.
 */
#ifdef CONFIG_NUMA
#define HASHDIST_DEFAULT IS_ENABLED(CONFIG_64BIT)
extern int hashdist;		/* Distribute hashes across NUMA nodes? */
#else
#define hashdist (0)
#endif

#ifdef CONFIG_MEMTEST
extern void early_memtest(phys_addr_t start, phys_addr_t end);
#else
static inline void early_memtest(phys_addr_t start, phys_addr_t end)
{
}
#endif

#endif /* __KERNEL__ */

#endif /* _LINUX_MEMBLOCK_H */
