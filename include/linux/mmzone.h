/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_MMZONE_H
#define _LINUX_MMZONE_H

#ifndef __ASSEMBLY__
#ifndef __GENERATING_BOUNDS_H

#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/bitops.h>
#include <linux/cache.h>
#include <linux/threads.h>
#include <linux/numa.h>
#include <linux/init.h>
#include <linux/seqlock.h>
#include <linux/nodemask.h>
#include <linux/pageblock-flags.h>
#include <linux/page-flags-layout.h>
#include <linux/atomic.h>
#include <linux/mm_types.h>
#include <linux/page-flags.h>
#include <asm/page.h>

/* Free memory management - zoned buddy allocator.  */
#ifndef CONFIG_FORCE_MAX_ZONEORDER
#define MAX_ORDER 11
#else
#define MAX_ORDER CONFIG_FORCE_MAX_ZONEORDER
#endif
#define MAX_ORDER_NR_PAGES (1 << (MAX_ORDER - 1))

/*
 * PAGE_ALLOC_COSTLY_ORDER is the order at which allocations are deemed
 * costly to service.  That is between allocation orders which should
 * coalesce naturally under reasonable reclaim pressure and those which
 * will not.
 */
#define PAGE_ALLOC_COSTLY_ORDER 3

enum migratetype {
/** comment by hy 2020-01-02
 * # 不可移动
 */
	MIGRATE_UNMOVABLE,
/** comment by hy 2020-01-02
 * # 可移动
 */
	MIGRATE_MOVABLE,
/** comment by hy 2020-01-02
 * # 可回收
 */
	MIGRATE_RECLAIMABLE,
/** comment by hy 2020-01-02
 * # 定义内存区域的每个处理器页集合中的链表的数量
 */
	MIGRATE_PCPTYPES,	/* the number of types on the pcp lists */
/** comment by hy 2020-01-02
 * # 高阶原子分配,即阶数大于0,并且分配页时,不能睡眠等待
 */
	MIGRATE_HIGHATOMIC = MIGRATE_PCPTYPES,
#ifdef CONFIG_CMA
	/*
	 * MIGRATE_CMA migration type is designed to mimic the way
	 * ZONE_MOVABLE works.  Only movable pages can be allocated
	 * from MIGRATE_CMA pageblocks and page allocator never
	 * implicitly change migration type of MIGRATE_CMA pageblock.
	 *
	 * The way to use it is to change migratetype of a range of
	 * pageblocks to MIGRATE_CMA which can be done by
	 * __free_pageblock_cma() function.  What is important though
	 * is that a range of pageblocks must be aligned to
	 * MAX_ORDER_NR_PAGES should biggest page be bigger then
	 * a single pageblock.
	 */
/** comment by hy 2020-01-02
 * # 连续内存分配器
 */
	MIGRATE_CMA,
#endif
#ifdef CONFIG_MEMORY_ISOLATION
/** comment by hy 2020-01-02
 * # 隔离不能从这里分配
        由连续内存分配器,内存热拔插,和内存硬件错误恢复等功能
 */
	MIGRATE_ISOLATE,	/* can't allocate from here */
#endif
	MIGRATE_TYPES
};

/* In mm/page_alloc.c; keep in sync also with show_migration_types() there */
extern const char * const migratetype_names[MIGRATE_TYPES];

#ifdef CONFIG_CMA
#  define is_migrate_cma(migratetype) unlikely((migratetype) == MIGRATE_CMA)
#  define is_migrate_cma_page(_page) (get_pageblock_migratetype(_page) == MIGRATE_CMA)
#else
#  define is_migrate_cma(migratetype) false
#  define is_migrate_cma_page(_page) false
#endif

static inline bool is_migrate_movable(int mt)
{
	return is_migrate_cma(mt) || mt == MIGRATE_MOVABLE;
}

#define for_each_migratetype_order(order, type) \
	for (order = 0; order < MAX_ORDER; order++) \
		for (type = 0; type < MIGRATE_TYPES; type++)

extern int page_group_by_mobility_disabled;

#define NR_MIGRATETYPE_BITS (PB_migrate_end - PB_migrate + 1)
#define MIGRATETYPE_MASK ((1UL << NR_MIGRATETYPE_BITS) - 1)

#define get_pageblock_migratetype(page)					\
	get_pfnblock_flags_mask(page, page_to_pfn(page),		\
			PB_migrate_end, MIGRATETYPE_MASK)

struct free_area {
/** comment by hy 2019-12-31
 * # 空闲页块的链表
 */
	struct list_head	free_list[MIGRATE_TYPES];
/** comment by hy 2019-12-31
 * # 空闲页块的数量
 */
	unsigned long		nr_free;
};

static inline struct page *get_page_from_free_area(struct free_area *area,
					    int migratetype)
{
	return list_first_entry_or_null(&area->free_list[migratetype],
					struct page, lru);
}

static inline bool free_area_empty(struct free_area *area, int migratetype)
{
	return list_empty(&area->free_list[migratetype]);
}

struct pglist_data;

/*
 * zone->lock and the zone lru_lock are two of the hottest locks in the kernel.
 * So add a wild amount of padding here to ensure that they fall into separate
 * cachelines.  There are very few zone structures in the machine, so space
 * consumption is not a concern here.
 */
#if defined(CONFIG_SMP)
struct zone_padding {
	char x[0];
} ____cacheline_internodealigned_in_smp;
#define ZONE_PADDING(name)	struct zone_padding name;
#else
#define ZONE_PADDING(name)
#endif

#ifdef CONFIG_NUMA
enum numa_stat_item {
	NUMA_HIT,		/* allocated in intended node */
	NUMA_MISS,		/* allocated in non intended node */
	NUMA_FOREIGN,		/* was intended here, hit elsewhere */
	NUMA_INTERLEAVE_HIT,	/* interleaver preferred this zone */
	NUMA_LOCAL,		/* allocation from local node */
	NUMA_OTHER,		/* allocation from other node */
	NR_VM_NUMA_STAT_ITEMS
};
#else
#define NR_VM_NUMA_STAT_ITEMS 0
#endif

enum zone_stat_item {
	/* First 128 byte cacheline (assuming 64 bit words) */
	NR_FREE_PAGES,
	NR_ZONE_LRU_BASE, /* Used only for compaction and reclaim retry */
	NR_ZONE_INACTIVE_ANON = NR_ZONE_LRU_BASE,
	NR_ZONE_ACTIVE_ANON,
	NR_ZONE_INACTIVE_FILE,
	NR_ZONE_ACTIVE_FILE,
	NR_ZONE_UNEVICTABLE,
	NR_ZONE_WRITE_PENDING,	/* Count of dirty, writeback and unstable pages */
	NR_MLOCK,		/* mlock()ed pages found and moved off LRU */
	NR_PAGETABLE,		/* used for pagetables */
	NR_KERNEL_STACK_KB,	/* measured in KiB */
#if IS_ENABLED(CONFIG_SHADOW_CALL_STACK)
	NR_KERNEL_SCS_KB,	/* measured in KiB */
#endif
	/* Second 128 byte cacheline */
	NR_BOUNCE,
#if IS_ENABLED(CONFIG_ZSMALLOC)
	NR_ZSPAGES,		/* allocated in zsmalloc */
#endif
	NR_FREE_CMA_PAGES,
	NR_VM_ZONE_STAT_ITEMS };

enum node_stat_item {
	NR_LRU_BASE,
	NR_INACTIVE_ANON = NR_LRU_BASE, /* must match order of LRU_[IN]ACTIVE */
	NR_ACTIVE_ANON,		/*  "     "     "   "       "         */
	NR_INACTIVE_FILE,	/*  "     "     "   "       "         */
	NR_ACTIVE_FILE,		/*  "     "     "   "       "         */
	NR_UNEVICTABLE,		/*  "     "     "   "       "         */
	NR_SLAB_RECLAIMABLE,
	NR_SLAB_UNRECLAIMABLE,
	NR_ISOLATED_ANON,	/* Temporary isolated pages from anon lru */
	NR_ISOLATED_FILE,	/* Temporary isolated pages from file lru */
	WORKINGSET_NODES,
	WORKINGSET_REFAULT,
	WORKINGSET_ACTIVATE,
	WORKINGSET_RESTORE,
	WORKINGSET_NODERECLAIM,
	NR_ANON_MAPPED,	/* Mapped anonymous pages */
	NR_FILE_MAPPED,	/* pagecache pages mapped into pagetables.
			   only modified from process context */
	NR_FILE_PAGES,
	NR_FILE_DIRTY,
	NR_WRITEBACK,
	NR_WRITEBACK_TEMP,	/* Writeback using temporary buffers */
	NR_SHMEM,		/* shmem pages (included tmpfs/GEM pages) */
	NR_SHMEM_THPS,
	NR_SHMEM_PMDMAPPED,
	NR_FILE_THPS,
	NR_FILE_PMDMAPPED,
	NR_ANON_THPS,
	NR_VMSCAN_WRITE,
	NR_VMSCAN_IMMEDIATE,	/* Prioritise for reclaim when writeback ends */
	NR_DIRTIED,		/* page dirtyings since bootup */
	NR_WRITTEN,		/* page writings since bootup */
	NR_KERNEL_MISC_RECLAIMABLE,	/* reclaimable non-slab kernel pages */
	NR_FOLL_PIN_ACQUIRED,	/* via: pin_user_page(), gup flag: FOLL_PIN */
	NR_FOLL_PIN_RELEASED,	/* pages returned via unpin_user_page() */
	NR_VM_NODE_STAT_ITEMS
};

/*
 * We do arithmetic on the LRU lists in various places in the code,
 * so it is important to keep the active lists LRU_ACTIVE higher in
 * the array than the corresponding inactive lists, and to keep
 * the *_FILE lists LRU_FILE higher than the corresponding _ANON lists.
 *
 * This has to be kept in sync with the statistics in zone_stat_item
 * above and the descriptions in vmstat_text in mm/vmstat.c
 */
#define LRU_BASE 0
#define LRU_ACTIVE 1
#define LRU_FILE 2

enum lru_list {
	LRU_INACTIVE_ANON = LRU_BASE,
	LRU_ACTIVE_ANON = LRU_BASE + LRU_ACTIVE,
	LRU_INACTIVE_FILE = LRU_BASE + LRU_FILE,
	LRU_ACTIVE_FILE = LRU_BASE + LRU_FILE + LRU_ACTIVE,
	LRU_UNEVICTABLE,
	NR_LRU_LISTS
};

#define for_each_lru(lru) for (lru = 0; lru < NR_LRU_LISTS; lru++)

#define for_each_evictable_lru(lru) for (lru = 0; lru <= LRU_ACTIVE_FILE; lru++)

static inline bool is_file_lru(enum lru_list lru)
{
	return (lru == LRU_INACTIVE_FILE || lru == LRU_ACTIVE_FILE);
}

static inline bool is_active_lru(enum lru_list lru)
{
	return (lru == LRU_ACTIVE_ANON || lru == LRU_ACTIVE_FILE);
}

enum lruvec_flags {
	LRUVEC_CONGESTED,		/* lruvec has many dirty pages
					 * backed by a congested BDI
					 */
};

struct lruvec {
	struct list_head		lists[NR_LRU_LISTS];
	/*
	 * These track the cost of reclaiming one LRU - file or anon -
	 * over the other. As the observed cost of reclaiming one LRU
	 * increases, the reclaim scan balance tips toward the other.
	 */
	unsigned long			anon_cost;
	unsigned long			file_cost;
	/* Non-resident age, driven by LRU movement */
	atomic_long_t			nonresident_age;
	/* Refaults at the time of last reclaim cycle */
	unsigned long			refaults;
	/* Various lruvec state flags (enum lruvec_flags) */
	unsigned long			flags;
#ifdef CONFIG_MEMCG
	struct pglist_data *pgdat;
#endif
};

/* Isolate unmapped pages */
#define ISOLATE_UNMAPPED	((__force isolate_mode_t)0x2)
/* Isolate for asynchronous migration */
#define ISOLATE_ASYNC_MIGRATE	((__force isolate_mode_t)0x4)
/* Isolate unevictable pages */
#define ISOLATE_UNEVICTABLE	((__force isolate_mode_t)0x8)

/* LRU Isolation modes. */
typedef unsigned __bitwise isolate_mode_t;

/** comment by hy 2020-01-02
 * # 计算低水线的方法和高水线的方法
        增量=(最低水线/4, zone->managed_pages * watermark_scale_factor / 100000)取最大值
        低水线=最低水线+增量
        高水线=最低水线+2*增量

        最低水线/4 比较大,那么计算公式简化如下
            低水线=最低水线 * 5/4
            高水线=最低水线 *3/2
 */
enum zone_watermarks {
/** comment by hy 2020-01-02
 * # 最低水平线,如果内存区域的空闲页数大于最低水线,说明内存域的内存严重不足
        = 4 * (low_kb)^(1/2)

        min_free_page = min_free_kbytes对应的页数
        zone->managed_pages = 该区伙伴分配器管理的页数
        lowmem_pages = 所有低端内存区域中伙伴分配器管理的页数总和
        高端内存区域的最低端水线=zone->managed_pages/1024,并且限制在范围[32,128]以内
        低端内存区域的最低水线=min_free_kbytes * zone->managed_pages /lowmem_pages
            即min_free_pages按照比例分配到每个低端内存区域
 */
	WMARK_MIN,
/** comment by hy 2020-01-02
 * # 低水线,如果内存区域的空闲页数小于低水线,说明内存域的内存轻微不足
    所有低端内存区域中伙伴分配器管理的总数
 */
	WMARK_LOW,
/** comment by hy 2020-01-02
 * # 高水线,如果内存区域的空闲页数大于高水线,说明内存域的内存充足
 */
	WMARK_HIGH,
	NR_WMARK
};

#define min_wmark_pages(z) (z->_watermark[WMARK_MIN] + z->watermark_boost)
#define low_wmark_pages(z) (z->_watermark[WMARK_LOW] + z->watermark_boost)
#define high_wmark_pages(z) (z->_watermark[WMARK_HIGH] + z->watermark_boost)
#define wmark_pages(z, i) (z->_watermark[i] + z->watermark_boost)

struct per_cpu_pages {
/** comment by hy 2020-01-04
 * # 链表里面的数量
 */
	int count;		/* number of pages in the list */
/** comment by hy 2020-01-04
 * # 页的数量达到高水线,需要返还给伙伴分配器
 */
	int high;		/* high watermark, emptying needed */
/** comment by hy 2020-01-04
 * # 批量添加或删除的页数量
 */
	int batch;		/* chunk size for buddy add/remove */

	/* Lists of pages, one per migrate type stored on the pcp-lists */
/** comment by hy 2020-01-04
 * # 每种迁移类型一个页链表
 */
	struct list_head lists[MIGRATE_PCPTYPES];
};

struct per_cpu_pageset {
	struct per_cpu_pages pcp;
#ifdef CONFIG_NUMA
	s8 expire;
	u16 vm_numa_stat_diff[NR_VM_NUMA_STAT_ITEMS];
#endif
#ifdef CONFIG_SMP
	s8 stat_threshold;
	s8 vm_stat_diff[NR_VM_ZONE_STAT_ITEMS];
#endif
};

struct per_cpu_nodestat {
	s8 stat_threshold;
	s8 vm_node_stat_diff[NR_VM_NODE_STAT_ITEMS];
};

#endif /* !__GENERATING_BOUNDS.H */

enum zone_type {
/** comment by hy 2019-12-30
 * # 直接访问区,有些设备不能访问所有内存,需要使用DMA区域
 */
	/*
	 * ZONE_DMA and ZONE_DMA32 are used when there are peripherals not able
	 * to DMA to all of the addressable memory (ZONE_NORMAL).
	 * On architectures where this area covers the whole 32 bit address
	 * space ZONE_DMA32 is used. ZONE_DMA is left for the ones with smaller
	 * DMA addressing constraints. This distinction is important as a 32bit
	 * DMA mask is assumed when ZONE_DMA32 is defined. Some 64-bit
	 * platforms may need both zones as they support peripherals with
	 * different DMA addressing limitations.
	 *
	 * Some examples:
	 *
	 *  - i386 and x86_64 have a fixed 16M ZONE_DMA and ZONE_DMA32 for the
	 *    rest of the lower 4G.
	 *
	 *  - arm only uses ZONE_DMA, the size, up to 4G, may vary depending on
	 *    the specific device.
	 *
	 *  - arm64 has a fixed 1G ZONE_DMA and ZONE_DMA32 for the rest of the
	 *    lower 4G.
	 *
	 *  - powerpc only uses ZONE_DMA, the size, up to 2G, may vary
	 *    depending on the specific device.
	 *
	 *  - s390 uses ZONE_DMA fixed to the lower 2G.
	 *
	 *  - ia64 and riscv only use ZONE_DMA32.
	 *
	 *  - parisc uses neither.
	 */
#ifdef CONFIG_ZONE_DMA
	ZONE_DMA,
#endif
#ifdef CONFIG_ZONE_DMA32
/** comment by hy 2019-12-30
 * # 支持访问16M以下的空间,又要支持访问4G以下的空间
 */
	ZONE_DMA32,
#endif
	/*
	 * Normal addressable memory is in ZONE_NORMAL. DMA operations can be
	 * performed on pages in ZONE_NORMAL if the DMA devices support
	 * transfers to all addressable memory.
	 */
/** comment by hy 2019-12-30
 * # 直接映射空间或线性映射空间(虚拟地址=物理地址+常量)
 */
	ZONE_NORMAL,
#ifdef CONFIG_HIGHMEM
	/*
	 * A memory area that is only addressable by the kernel through
	 * mapping portions into its own address space. This is for example
	 * used by i386 to allow the kernel to address the memory beyond
	 * 900MB. The kernel will set up special mappings (page
	 * table entries on i386) for each page that the kernel needs to
	 * access.
	 */
/** comment by hy 2019-12-30
 * # 32位系统内核和用户空间安装1:3进行划分
     64位系统虚拟地址空间非常大,不再需要高端内存区域
 */
	ZONE_HIGHMEM,
#endif
/** comment by hy 2019-12-30
 * # 伪内存区域,用来防止内存碎片化
 */
	ZONE_MOVABLE,
#ifdef CONFIG_ZONE_DEVICE
/** comment by hy 2019-12-30
 * # 设备区域,用来支撑持久内存的热拔插增加的内存区域
 */
	ZONE_DEVICE,
#endif
	__MAX_NR_ZONES

};

#ifndef __GENERATING_BOUNDS_H

struct zone {
	/* Read-mostly fields */

	/* zone watermarks, access with *_wmark_pages(zone) macros */
/** comment by hy 2019-12-30
 * # 页分配器使用的水线
 */
	unsigned long _watermark[NR_WMARK];
/** comment by hy 2019-12-30
 * # 页分配器使用,当前区域保留多少页不能借给高的区域类型
 */
	unsigned long watermark_boost;
/** comment by hy 2020-01-04
 * # 记录高阶原子类型的总页数,并且限制数量
        改值 小于 managed_pages / 100 + pageblock_nr_page
        即必须小于 伙伴分配器管理的总页数/100 + 分组阶数对应的页数
 */
	unsigned long nr_reserved_highatomic;

	/*
	 * We don't know if the memory that we're going to allocate will be
	 * freeable or/and it will be released eventually, so to avoid totally
	 * wasting several GB of ram we must reserve some of the lower zone
	 * memory (otherwise we risk to run OOM on the lower zones despite
	 * there being tons of freeable ram on the higher zones).  This array is
	 * recalculated at runtime if the sysctl_lowmem_reserve_ratio sysctl
	 * changes.
	 */
/** comment by hy 2020-01-02
 * # 表示区域类型i应该保留多少页不能借给区域类型j,仅当j大于i时有意义
     sysctrl_lowmem_reserve_ratio 存放各种区域类型的保留比例,因为内核不允许
     使用浮点数,所以使用倒数
     DMA区域与DMA32区域默认保留256,普通区域和高端内存区域的默认保留比例都是32
        i < j:
          zone[i].lowmem_reserve[i] = (当前内存节点在zone[i+1]到zone[j]伙伴分配器
            管理的页数总和)/sysctrl_lowmem_reserve_ratio[i]
        i = j or i > j:
          zone[i].lowmem_reserve[i] = 0
 */
	long lowmem_reserve[MAX_NR_ZONES];

#ifdef CONFIG_NUMA
	int node;
#endif
/** comment by hy 2019-12-30
 * # 指向内存节点的pglist_data实例
 */
	struct pglist_data	*zone_pgdat;
/** comment by hy 2019-12-30
 * # 每处理器页的集合
        页集合中每种迁移类型有一个页链表
        页集合含有高水线和批量值,页集合中的页数量不能超过高水线
        申请单页加入页链表,或者从页链表返还伙伴分配器,都是采用批量操作
        一次操作的页数量是批量值
     默认的批量值计算方法：
        1：batch=zone->managed_pages/1024
            其中zone->managed_pages是内存区域中由伙伴分配器管理的页数量
        2：如果batch超过(512 *1024)/PAGE_SIZE, 那么把batch设置为(512*1024)
            /PAGE_SIZE,其中PAGE_SIZE是页长度
        3：batch=batch/4
        4：如果batch小于1,那么设置为1
        5：batch = rounddown_pow_of_two(batch * 1.5) - 1,其中rounddown_pow_of_two
            是把数值向下对其到2的N次幂
        默认高水线是批量值的6倍
        通过修改文件/proc/sys/vm/percpu_pagelist_fraction修改比例值,最小值为8
        高水线等于(伙伴分配器管理页数量/比例值),同时把批量值设置为高水线的1/4
 */
	struct per_cpu_pageset __percpu *pageset;

#ifndef CONFIG_SPARSEMEM
	/*
	 * Flags for a pageblock_nr_pages block. See pageblock-flags.h.
	 * In SPARSEMEM, this map is stored in struct mem_section
	 */
/** comment by hy 2020-01-03
 * # 分组页块的标志参考文件pageblock-flags.h
        如果使用稀疏内存模型,这个位图在结构体 mem_section 中
 */
	unsigned long		*pageblock_flags;
#endif /* CONFIG_SPARSEMEM */

	/* zone_start_pfn == zone_start_paddr >> PAGE_SHIFT */
/** comment by hy 2019-12-30
 * # 当前区域的起始物理页号
 */
	unsigned long		zone_start_pfn;

	/*
	 * spanned_pages is the total pages spanned by the zone, including
	 * holes, which is calculated as:
	 * 	spanned_pages = zone_end_pfn - zone_start_pfn;
	 *
	 * present_pages is physical pages existing within the zone, which
	 * is calculated as:
	 *	present_pages = spanned_pages - absent_pages(pages in holes);
	 *
	 * managed_pages is present pages managed by the buddy system, which
	 * is calculated as (reserved_pages includes pages allocated by the
	 * bootmem allocator):
	 *	managed_pages = present_pages - reserved_pages;
	 *
	 * So present_pages may be used by memory hotplug or memory power
	 * management logic to figure out unmanaged pages by checking
	 * (present_pages - managed_pages). And managed_pages should be used
	 * by page allocator and vm scanner to calculate all kinds of watermarks
	 * and thresholds.
	 *
	 * Locking rules:
	 *
	 * zone_start_pfn and spanned_pages are protected by span_seqlock.
	 * It is a seqlock because it has to be read outside of zone->lock,
	 * and it is done in the main allocator path.  But, it is written
	 * quite infrequently.
	 *
	 * The span_seq lock is declared along with zone->lock because it is
	 * frequently read in proximity to zone->lock.  It's good to
	 * give them a chance of being in the same cacheline.
	 *
	 * Write access to present_pages at runtime should be protected by
	 * mem_hotplug_begin/end(). Any reader who can't tolerant drift of
	 * present_pages should get_online_mems() to get a stable value.
	 */
/** comment by hy 2019-12-30
 * # 伙伴分配器管理的物理页的数量
        不包括内存分配器分配的物理页
 */
	atomic_long_t		managed_pages;
/** comment by hy 2019-12-30
 * # 当前区跨越的总页数,包括空洞
 */
	unsigned long		spanned_pages;
/** comment by hy 2019-12-30
 * # 当前区域存在的物理页的数量,不包括空洞
 */
	unsigned long		present_pages;
/** comment by hy 2019-12-30
 * # 分区的名称
 */
	const char		*name;

#ifdef CONFIG_MEMORY_ISOLATION
	/*
	 * Number of isolated pageblock. It is used to solve incorrect
	 * freepage counting problem due to racy retrieving migratetype
	 * of pageblock. Protected by zone->lock.
	 */
	unsigned long		nr_isolate_pageblock;
#endif

#ifdef CONFIG_MEMORY_HOTPLUG
	/* see spanned/present_pages for more description */
	seqlock_t		span_seqlock;
#endif

	int initialized;

	/* Write-intensive fields used from the page allocator */
	ZONE_PADDING(_pad1_)

	/* free areas of different sizes */
/** comment by hy 2019-12-30
 * # 不同长度的空闲区域
        在伙伴分配器,维护空闲页块,数组下标对应页块的阶数
 */
	struct free_area	free_area[MAX_ORDER];

	/* zone flags, see below */
	unsigned long		flags;

	/* Primarily protects free_area */
	spinlock_t		lock;

	/* Write-intensive fields used by compaction and vmstats. */
	ZONE_PADDING(_pad2_)

	/*
	 * When free pages are below this point, additional steps are taken
	 * when reading the number of free pages to avoid per-cpu counter
	 * drift allowing watermarks to be breached
	 */
	unsigned long percpu_drift_mark;

#if defined CONFIG_COMPACTION || defined CONFIG_CMA
	/* pfn where compaction free scanner should start */
	unsigned long		compact_cached_free_pfn;
	/* pfn where async and sync compaction migration scanner should start */
	unsigned long		compact_cached_migrate_pfn[2];
	unsigned long		compact_init_migrate_pfn;
	unsigned long		compact_init_free_pfn;
#endif

#ifdef CONFIG_COMPACTION
	/*
	 * On compaction failure, 1<<compact_defer_shift compactions
	 * are skipped before trying again. The number attempted since
	 * last failure is tracked with compact_considered.
	 */
	unsigned int		compact_considered;
	unsigned int		compact_defer_shift;
	int			compact_order_failed;
#endif

#if defined CONFIG_COMPACTION || defined CONFIG_CMA
	/* Set to true when the PG_migrate_skip bits should be cleared */
	bool			compact_blockskip_flush;
#endif

	bool			contiguous;

	ZONE_PADDING(_pad3_)
	/* Zone statistics */
	atomic_long_t		vm_stat[NR_VM_ZONE_STAT_ITEMS];
	atomic_long_t		vm_numa_stat[NR_VM_NUMA_STAT_ITEMS];
} ____cacheline_internodealigned_in_smp;

enum pgdat_flags {
	PGDAT_DIRTY,			/* reclaim scanning has recently found
					 * many dirty file pages at the tail
					 * of the LRU.
					 */
	PGDAT_WRITEBACK,		/* reclaim scanning has recently found
					 * many pages under writeback
					 */
	PGDAT_RECLAIM_LOCKED,		/* prevents concurrent reclaim */
};

enum zone_flags {
	ZONE_BOOSTED_WATERMARK,		/* zone recently boosted watermarks.
					 * Cleared when kswapd is woken.
					 */
};

static inline unsigned long zone_managed_pages(struct zone *zone)
{
	return (unsigned long)atomic_long_read(&zone->managed_pages);
}

static inline unsigned long zone_end_pfn(const struct zone *zone)
{
	return zone->zone_start_pfn + zone->spanned_pages;
}

static inline bool zone_spans_pfn(const struct zone *zone, unsigned long pfn)
{
	return zone->zone_start_pfn <= pfn && pfn < zone_end_pfn(zone);
}

static inline bool zone_is_initialized(struct zone *zone)
{
	return zone->initialized;
}

static inline bool zone_is_empty(struct zone *zone)
{
	return zone->spanned_pages == 0;
}

/*
 * Return true if [start_pfn, start_pfn + nr_pages) range has a non-empty
 * intersection with the given zone
 */
static inline bool zone_intersects(struct zone *zone,
		unsigned long start_pfn, unsigned long nr_pages)
{
	if (zone_is_empty(zone))
		return false;
	if (start_pfn >= zone_end_pfn(zone) ||
	    start_pfn + nr_pages <= zone->zone_start_pfn)
		return false;

	return true;
}

/*
 * The "priority" of VM scanning is how much of the queues we will scan in one
 * go. A value of 12 for DEF_PRIORITY implies that we will scan 1/4096th of the
 * queues ("queue_length >> 12") during an aging round.
 */
#define DEF_PRIORITY 12

/* Maximum number of zones on a zonelist */
#define MAX_ZONES_PER_ZONELIST (MAX_NUMNODES * MAX_NR_ZONES)

enum {
/** comment by hy 2020-01-02
 * # 包含所有内存节点的备用区域列表
 */
	ZONELIST_FALLBACK,	/* zonelist with fallback */
#ifdef CONFIG_NUMA
	/*
	 * The NUMA zonelists are doubled because we need zonelists that
	 * restrict the allocations to a single node for __GFP_THISNODE.
	 */
/** comment by hy 2020-01-02
 * # 只包含当前内存节点的备用区域列表
 */
	ZONELIST_NOFALLBACK,	/* zonelist without fallback (__GFP_THISNODE) */
#endif
	MAX_ZONELISTS
};

/*
 * This struct contains information about a zone in a zonelist. It is stored
 * here to avoid dereferences into large structures and lookups of tables
 */
struct zoneref {
/** comment by hy 2020-01-02
 * # 指向内存区域的数据结构
 */
	struct zone *zone;	/* Pointer to actual zone */
/** comment by hy 2020-01-02
 * # 成员zone指向的内存区域类型
 */
	int zone_idx;		/* zone_idx(zoneref->zone) */
};

/*
 * One allocation request operates on a zonelist. A zonelist
 * is a list of zones, the first one is the 'goal' of the
 * allocation, the other zones are fallback zones, in decreasing
 * priority.
 *
 * To speed the reading of the zonelist, the zonerefs contain the zone index
 * of the entry being read. Helper functions to access information given
 * a struct zoneref are
 *
 * zonelist_zone()	- Return the struct zone * for an entry in _zonerefs
 * zonelist_zone_idx()	- Return the index of the zone for an entry
 * zonelist_node_idx()	- Return the index of the node for an entry
 */
struct zonelist {
	struct zoneref _zonerefs[MAX_ZONES_PER_ZONELIST + 1];
};

#ifndef CONFIG_DISCONTIGMEM
/* The array of struct pages - for discontigmem use pgdat->lmem_map */
extern struct page *mem_map;
#endif

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
struct deferred_split {
	spinlock_t split_queue_lock;
	struct list_head split_queue;
	unsigned long split_queue_len;
};
#endif

/*
 * On NUMA machines, each NUMA node would have a pg_data_t to describe
 * it's memory layout. On UMA machines there is a single pglist_data which
 * describes the whole memory.
 *
 * Memory statistics and page replacement data structures are maintained on a
 * per-zone basis.
 */
typedef struct pglist_data {
/** comment by hy 2019-12-27
 * # 内存区域数组
 */
	struct zone node_zones[MAX_NR_ZONES];
/** comment by hy 2019-12-27
 * # 备用区域列表
 */
	struct zonelist node_zonelists[MAX_ZONELISTS];
/** comment by hy 2019-12-27
 * # 内存区域数量
 */
	int nr_zones;
	/*
	 * node_zones contains just the zones for THIS node. Not all of the
	 * zones may be populated, but it is the full list. It is referenced by
	 * this node's node_zonelists as well as other node's node_zonelists.
	 */

	/*
	 * node_zonelists contains references to all zones in all nodes.
	 * Generally the first zones will be references to this node's
	 * node_zones.
	 */

#ifdef CONFIG_FLAT_NODE_MEM_MAP	/* means !SPARSEMEM */
/** comment by hy 2019-12-27
 * # 页描述符数组
        每个物理页对应一个页描述符
        页描述符数组的大小必须对其到 MAX_ORDER-1 幂次上
        MAX_ORDER-1 是页分配器可分配的最大阶数
 */
	struct page *node_mem_map;
#ifdef CONFIG_PAGE_EXTENSION
	struct page_ext *node_page_ext;
#endif
#endif
#if defined(CONFIG_MEMORY_HOTPLUG) || defined(CONFIG_DEFERRED_STRUCT_PAGE_INIT)
	/*
	 * Must be held any time you expect node_start_pfn,
	 * node_present_pages, node_spanned_pages or nr_zones to stay constant.
	 * Also synchronizes pgdat->first_deferred_pfn during deferred page
	 * init.
	 *
	 * pgdat_resize_lock() and pgdat_resize_unlock() are provided to
	 * manipulate node_size_lock without checking for CONFIG_MEMORY_HOTPLUG
	 * or CONFIG_DEFERRED_STRUCT_PAGE_INIT.
	 *
	 * Nests above zone->lock and zone->span_seqlock
	 */
	spinlock_t node_size_lock;
#endif
/** comment by hy 2019-12-27
 * # 起始物理页号
 */
	unsigned long node_start_pfn;
/** comment by hy 2019-12-27
 * # 物理页总数(不包括空洞)
 */
	unsigned long node_present_pages; /* total number of physical pages */
/** comment by hy 2019-12-27
 * # 物理页总数(包括空洞)
 */
	unsigned long node_spanned_pages; /* total size of physical page
					     range, including holes */
/** comment by hy 2019-12-27
 * # 节点标识符
 */
	int node_id;
	wait_queue_head_t kswapd_wait;
	wait_queue_head_t pfmemalloc_wait;
	struct task_struct *kswapd;	/* Protected by
					   mem_hotplug_begin/end() */
	int kswapd_order;
	enum zone_type kswapd_highest_zoneidx;

	int kswapd_failures;		/* Number of 'reclaimed == 0' runs */

#ifdef CONFIG_COMPACTION
	int kcompactd_max_order;
	enum zone_type kcompactd_highest_zoneidx;
	wait_queue_head_t kcompactd_wait;
	struct task_struct *kcompactd;
#endif
	/*
	 * This is a per-node reserve of pages that are not available
	 * to userspace allocations.
	 */
	unsigned long		totalreserve_pages;

#ifdef CONFIG_NUMA
	/*
	 * node reclaim becomes active if more unmapped pages exist.
	 */
	unsigned long		min_unmapped_pages;
	unsigned long		min_slab_pages;
#endif /* CONFIG_NUMA */

	/* Write-intensive fields used by page reclaim */
	ZONE_PADDING(_pad1_)
	spinlock_t		lru_lock;

#ifdef CONFIG_DEFERRED_STRUCT_PAGE_INIT
	/*
	 * If memory initialisation on large machines is deferred then this
	 * is the first PFN that needs to be initialised.
	 */
	unsigned long first_deferred_pfn;
#endif /* CONFIG_DEFERRED_STRUCT_PAGE_INIT */

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
	struct deferred_split deferred_split_queue;
#endif

	/* Fields commonly accessed by the page reclaim scanner */

	/*
	 * NOTE: THIS IS UNUSED IF MEMCG IS ENABLED.
	 *
	 * Use mem_cgroup_lruvec() to look up lruvecs.
	 */
	struct lruvec		__lruvec;

	unsigned long		flags;

	ZONE_PADDING(_pad2_)

	/* Per-node vmstats */
	struct per_cpu_nodestat __percpu *per_cpu_nodestats;
	atomic_long_t		vm_stat[NR_VM_NODE_STAT_ITEMS];
} pg_data_t;

#define node_present_pages(nid)	(NODE_DATA(nid)->node_present_pages)
#define node_spanned_pages(nid)	(NODE_DATA(nid)->node_spanned_pages)
#ifdef CONFIG_FLAT_NODE_MEM_MAP
#define pgdat_page_nr(pgdat, pagenr)	((pgdat)->node_mem_map + (pagenr))
#else
#define pgdat_page_nr(pgdat, pagenr)	pfn_to_page((pgdat)->node_start_pfn + (pagenr))
#endif
#define nid_page_nr(nid, pagenr) 	pgdat_page_nr(NODE_DATA(nid),(pagenr))

#define node_start_pfn(nid)	(NODE_DATA(nid)->node_start_pfn)
#define node_end_pfn(nid) pgdat_end_pfn(NODE_DATA(nid))

static inline unsigned long pgdat_end_pfn(pg_data_t *pgdat)
{
	return pgdat->node_start_pfn + pgdat->node_spanned_pages;
}

static inline bool pgdat_is_empty(pg_data_t *pgdat)
{
	return !pgdat->node_start_pfn && !pgdat->node_spanned_pages;
}

#include <linux/memory_hotplug.h>

void build_all_zonelists(pg_data_t *pgdat);
void wakeup_kswapd(struct zone *zone, gfp_t gfp_mask, int order,
		   enum zone_type highest_zoneidx);
bool __zone_watermark_ok(struct zone *z, unsigned int order, unsigned long mark,
			 int highest_zoneidx, unsigned int alloc_flags,
			 long free_pages);
bool zone_watermark_ok(struct zone *z, unsigned int order,
		unsigned long mark, int highest_zoneidx,
		unsigned int alloc_flags);
bool zone_watermark_ok_safe(struct zone *z, unsigned int order,
		unsigned long mark, int highest_zoneidx);
enum memmap_context {
	MEMMAP_EARLY,
	MEMMAP_HOTPLUG,
};
extern void init_currently_empty_zone(struct zone *zone, unsigned long start_pfn,
				     unsigned long size);

extern void lruvec_init(struct lruvec *lruvec);

static inline struct pglist_data *lruvec_pgdat(struct lruvec *lruvec)
{
#ifdef CONFIG_MEMCG
	return lruvec->pgdat;
#else
	return container_of(lruvec, struct pglist_data, __lruvec);
#endif
}

extern unsigned long lruvec_lru_size(struct lruvec *lruvec, enum lru_list lru, int zone_idx);

#ifdef CONFIG_HAVE_MEMORY_PRESENT
void memory_present(int nid, unsigned long start, unsigned long end);
#else
static inline void memory_present(int nid, unsigned long start, unsigned long end) {}
#endif

#if defined(CONFIG_SPARSEMEM)
void memblocks_present(void);
#else
static inline void memblocks_present(void) {}
#endif

#ifdef CONFIG_HAVE_MEMORYLESS_NODES
int local_memory_node(int node_id);
#else
static inline int local_memory_node(int node_id) { return node_id; };
#endif

/*
 * zone_idx() returns 0 for the ZONE_DMA zone, 1 for the ZONE_NORMAL zone, etc.
 */
#define zone_idx(zone)		((zone) - (zone)->zone_pgdat->node_zones)

/*
 * Returns true if a zone has pages managed by the buddy allocator.
 * All the reclaim decisions have to use this function rather than
 * populated_zone(). If the whole zone is reserved then we can easily
 * end up with populated_zone() && !managed_zone().
 */
static inline bool managed_zone(struct zone *zone)
{
	return zone_managed_pages(zone);
}

/* Returns true if a zone has memory */
static inline bool populated_zone(struct zone *zone)
{
	return zone->present_pages;
}

#ifdef CONFIG_NUMA
static inline int zone_to_nid(struct zone *zone)
{
	return zone->node;
}

static inline void zone_set_nid(struct zone *zone, int nid)
{
	zone->node = nid;
}
#else
static inline int zone_to_nid(struct zone *zone)
{
	return 0;
}

static inline void zone_set_nid(struct zone *zone, int nid) {}
#endif

extern int movable_zone;

#ifdef CONFIG_HIGHMEM
static inline int zone_movable_is_highmem(void)
{
#ifdef CONFIG_NEED_MULTIPLE_NODES
	return movable_zone == ZONE_HIGHMEM;
#else
	return (ZONE_MOVABLE - 1) == ZONE_HIGHMEM;
#endif
}
#endif

static inline int is_highmem_idx(enum zone_type idx)
{
#ifdef CONFIG_HIGHMEM
	return (idx == ZONE_HIGHMEM ||
		(idx == ZONE_MOVABLE && zone_movable_is_highmem()));
#else
	return 0;
#endif
}

/**
 * is_highmem - helper function to quickly check if a struct zone is a
 *              highmem zone or not.  This is an attempt to keep references
 *              to ZONE_{DMA/NORMAL/HIGHMEM/etc} in general code to a minimum.
 * @zone - pointer to struct zone variable
 */
static inline int is_highmem(struct zone *zone)
{
#ifdef CONFIG_HIGHMEM
	return is_highmem_idx(zone_idx(zone));
#else
	return 0;
#endif
}

/* These two functions are used to setup the per zone pages min values */
struct ctl_table;

int min_free_kbytes_sysctl_handler(struct ctl_table *, int, void *, size_t *,
		loff_t *);
int watermark_scale_factor_sysctl_handler(struct ctl_table *, int, void *,
		size_t *, loff_t *);
extern int sysctl_lowmem_reserve_ratio[MAX_NR_ZONES];
int lowmem_reserve_ratio_sysctl_handler(struct ctl_table *, int, void *,
		size_t *, loff_t *);
int percpu_pagelist_fraction_sysctl_handler(struct ctl_table *, int,
		void *, size_t *, loff_t *);
int sysctl_min_unmapped_ratio_sysctl_handler(struct ctl_table *, int,
		void *, size_t *, loff_t *);
int sysctl_min_slab_ratio_sysctl_handler(struct ctl_table *, int,
		void *, size_t *, loff_t *);
int numa_zonelist_order_handler(struct ctl_table *, int,
		void *, size_t *, loff_t *);
extern int percpu_pagelist_fraction;
extern char numa_zonelist_order[];
#define NUMA_ZONELIST_ORDER_LEN	16

#ifndef CONFIG_NEED_MULTIPLE_NODES

extern struct pglist_data contig_page_data;
#define NODE_DATA(nid)		(&contig_page_data)
#define NODE_MEM_MAP(nid)	mem_map

#else /* CONFIG_NEED_MULTIPLE_NODES */

#include <asm/mmzone.h>

#endif /* !CONFIG_NEED_MULTIPLE_NODES */

extern struct pglist_data *first_online_pgdat(void);
extern struct pglist_data *next_online_pgdat(struct pglist_data *pgdat);
extern struct zone *next_zone(struct zone *zone);

/**
 * for_each_online_pgdat - helper macro to iterate over all online nodes
 * @pgdat - pointer to a pg_data_t variable
 */
#define for_each_online_pgdat(pgdat)			\
	for (pgdat = first_online_pgdat();		\
	     pgdat;					\
	     pgdat = next_online_pgdat(pgdat))
/**
 * for_each_zone - helper macro to iterate over all memory zones
 * @zone - pointer to struct zone variable
 *
 * The user only needs to declare the zone variable, for_each_zone
 * fills it in.
 */
#define for_each_zone(zone)			        \
	for (zone = (first_online_pgdat())->node_zones; \
	     zone;					\
	     zone = next_zone(zone))

#define for_each_populated_zone(zone)		        \
	for (zone = (first_online_pgdat())->node_zones; \
	     zone;					\
	     zone = next_zone(zone))			\
		if (!populated_zone(zone))		\
			; /* do nothing */		\
		else

static inline struct zone *zonelist_zone(struct zoneref *zoneref)
{
	return zoneref->zone;
}

static inline int zonelist_zone_idx(struct zoneref *zoneref)
{
	return zoneref->zone_idx;
}

static inline int zonelist_node_idx(struct zoneref *zoneref)
{
	return zone_to_nid(zoneref->zone);
}

struct zoneref *__next_zones_zonelist(struct zoneref *z,
					enum zone_type highest_zoneidx,
					nodemask_t *nodes);

/**
 * next_zones_zonelist - Returns the next zone at or below highest_zoneidx within the allowed nodemask using a cursor within a zonelist as a starting point
 * @z - The cursor used as a starting point for the search
 * @highest_zoneidx - The zone index of the highest zone to return
 * @nodes - An optional nodemask to filter the zonelist with
 *
 * This function returns the next zone at or below a given zone index that is
 * within the allowed nodemask using a cursor as the starting point for the
 * search. The zoneref returned is a cursor that represents the current zone
 * being examined. It should be advanced by one before calling
 * next_zones_zonelist again.
 */
static __always_inline struct zoneref *next_zones_zonelist(struct zoneref *z,
					enum zone_type highest_zoneidx,
					nodemask_t *nodes)
{
	if (likely(!nodes && zonelist_zone_idx(z) <= highest_zoneidx))
		return z;
	return __next_zones_zonelist(z, highest_zoneidx, nodes);
}

/**
 * first_zones_zonelist - Returns the first zone at or below highest_zoneidx within the allowed nodemask in a zonelist
 * @zonelist - The zonelist to search for a suitable zone
 * @highest_zoneidx - The zone index of the highest zone to return
 * @nodes - An optional nodemask to filter the zonelist with
 * @return - Zoneref pointer for the first suitable zone found (see below)
 *
 * This function returns the first zone at or below a given zone index that is
 * within the allowed nodemask. The zoneref returned is a cursor that can be
 * used to iterate the zonelist with next_zones_zonelist by advancing it by
 * one before calling.
 *
 * When no eligible zone is found, zoneref->zone is NULL (zoneref itself is
 * never NULL). This may happen either genuinely, or due to concurrent nodemask
 * update due to cpuset modification.
 */
static inline struct zoneref *first_zones_zonelist(struct zonelist *zonelist,
					enum zone_type highest_zoneidx,
					nodemask_t *nodes)
{
	return next_zones_zonelist(zonelist->_zonerefs,
							highest_zoneidx, nodes);
}

/**
 * for_each_zone_zonelist_nodemask - helper macro to iterate over valid zones in a zonelist at or below a given zone index and within a nodemask
 * @zone - The current zone in the iterator
 * @z - The current pointer within zonelist->_zonerefs being iterated
 * @zlist - The zonelist being iterated
 * @highidx - The zone index of the highest zone to return
 * @nodemask - Nodemask allowed by the allocator
 *
 * This iterator iterates though all zones at or below a given zone index and
 * within a given nodemask
 */
#define for_each_zone_zonelist_nodemask(zone, z, zlist, highidx, nodemask) \
	for (z = first_zones_zonelist(zlist, highidx, nodemask), zone = zonelist_zone(z);	\
		zone;							\
		z = next_zones_zonelist(++z, highidx, nodemask),	\
			zone = zonelist_zone(z))

#define for_next_zone_zonelist_nodemask(zone, z, zlist, highidx, nodemask) \
	for (zone = z->zone;	\
		zone;							\
		z = next_zones_zonelist(++z, highidx, nodemask),	\
			zone = zonelist_zone(z))


/**
 * for_each_zone_zonelist - helper macro to iterate over valid zones in a zonelist at or below a given zone index
 * @zone - The current zone in the iterator
 * @z - The current pointer within zonelist->zones being iterated
 * @zlist - The zonelist being iterated
 * @highidx - The zone index of the highest zone to return
 *
 * This iterator iterates though all zones at or below a given zone index.
 */
#define for_each_zone_zonelist(zone, z, zlist, highidx) \
	for_each_zone_zonelist_nodemask(zone, z, zlist, highidx, NULL)

#ifdef CONFIG_SPARSEMEM
#include <asm/sparsemem.h>
#endif

#ifdef CONFIG_FLATMEM
#define pfn_to_nid(pfn)		(0)
#endif

#ifdef CONFIG_SPARSEMEM

/*
 * SECTION_SHIFT    		#bits space required to store a section #
 *
 * PA_SECTION_SHIFT		physical address to/from section number
 * PFN_SECTION_SHIFT		pfn to/from section number
 */
#define PA_SECTION_SHIFT	(SECTION_SIZE_BITS)
#define PFN_SECTION_SHIFT	(SECTION_SIZE_BITS - PAGE_SHIFT)

#define NR_MEM_SECTIONS		(1UL << SECTIONS_SHIFT)

#define PAGES_PER_SECTION       (1UL << PFN_SECTION_SHIFT)
#define PAGE_SECTION_MASK	(~(PAGES_PER_SECTION-1))

#define SECTION_BLOCKFLAGS_BITS \
	((1UL << (PFN_SECTION_SHIFT - pageblock_order)) * NR_PAGEBLOCK_BITS)

#if (MAX_ORDER - 1 + PAGE_SHIFT) > SECTION_SIZE_BITS
#error Allocator MAX_ORDER exceeds SECTION_SIZE
#endif

static inline unsigned long pfn_to_section_nr(unsigned long pfn)
{
	return pfn >> PFN_SECTION_SHIFT;
}
static inline unsigned long section_nr_to_pfn(unsigned long sec)
{
	return sec << PFN_SECTION_SHIFT;
}

#define SECTION_ALIGN_UP(pfn)	(((pfn) + PAGES_PER_SECTION - 1) & PAGE_SECTION_MASK)
#define SECTION_ALIGN_DOWN(pfn)	((pfn) & PAGE_SECTION_MASK)

#define SUBSECTION_SHIFT 21
#define SUBSECTION_SIZE (1UL << SUBSECTION_SHIFT)

#define PFN_SUBSECTION_SHIFT (SUBSECTION_SHIFT - PAGE_SHIFT)
#define PAGES_PER_SUBSECTION (1UL << PFN_SUBSECTION_SHIFT)
#define PAGE_SUBSECTION_MASK (~(PAGES_PER_SUBSECTION-1))

#if SUBSECTION_SHIFT > SECTION_SIZE_BITS
#error Subsection size exceeds section size
#else
#define SUBSECTIONS_PER_SECTION (1UL << (SECTION_SIZE_BITS - SUBSECTION_SHIFT))
#endif

#define SUBSECTION_ALIGN_UP(pfn) ALIGN((pfn), PAGES_PER_SUBSECTION)
#define SUBSECTION_ALIGN_DOWN(pfn) ((pfn) & PAGE_SUBSECTION_MASK)

struct mem_section_usage {
#ifdef CONFIG_SPARSEMEM_VMEMMAP
	DECLARE_BITMAP(subsection_map, SUBSECTIONS_PER_SECTION);
#endif
	/* See declaration of similar field in struct zone */
	unsigned long pageblock_flags[0];
};

void subsection_map_init(unsigned long pfn, unsigned long nr_pages);

struct page;
struct page_ext;
struct mem_section {
	/*
	 * This is, logically, a pointer to an array of struct
	 * pages.  However, it is stored with some other magic.
	 * (see sparse.c::sparse_init_one_section())
	 *
	 * Additionally during early boot we encode node id of
	 * the location of the section here to guide allocation.
	 * (see sparse.c::memory_present())
	 *
	 * Making it a UL at least makes someone do a cast
	 * before using it wrong.
	 */
	unsigned long section_mem_map;

	struct mem_section_usage *usage;
#ifdef CONFIG_PAGE_EXTENSION
	/*
	 * If SPARSEMEM, pgdat doesn't have page_ext pointer. We use
	 * section. (see page_ext.h about this.)
	 */
	struct page_ext *page_ext;
	unsigned long pad;
#endif
	/*
	 * WARNING: mem_section must be a power-of-2 in size for the
	 * calculation and use of SECTION_ROOT_MASK to make sense.
	 */
};

#ifdef CONFIG_SPARSEMEM_EXTREME
#define SECTIONS_PER_ROOT       (PAGE_SIZE / sizeof (struct mem_section))
#else
#define SECTIONS_PER_ROOT	1
#endif

#define SECTION_NR_TO_ROOT(sec)	((sec) / SECTIONS_PER_ROOT)
#define NR_SECTION_ROOTS	DIV_ROUND_UP(NR_MEM_SECTIONS, SECTIONS_PER_ROOT)
#define SECTION_ROOT_MASK	(SECTIONS_PER_ROOT - 1)

#ifdef CONFIG_SPARSEMEM_EXTREME
extern struct mem_section **mem_section;
#else
extern struct mem_section mem_section[NR_SECTION_ROOTS][SECTIONS_PER_ROOT];
#endif

static inline unsigned long *section_to_usemap(struct mem_section *ms)
{
	return ms->usage->pageblock_flags;
}

static inline struct mem_section *__nr_to_section(unsigned long nr)
{
#ifdef CONFIG_SPARSEMEM_EXTREME
	if (!mem_section)
		return NULL;
#endif
	if (!mem_section[SECTION_NR_TO_ROOT(nr)])
		return NULL;
	return &mem_section[SECTION_NR_TO_ROOT(nr)][nr & SECTION_ROOT_MASK];
}
extern unsigned long __section_nr(struct mem_section *ms);
extern size_t mem_section_usage_size(void);

/*
 * We use the lower bits of the mem_map pointer to store
 * a little bit of information.  The pointer is calculated
 * as mem_map - section_nr_to_pfn(pnum).  The result is
 * aligned to the minimum alignment of the two values:
 *   1. All mem_map arrays are page-aligned.
 *   2. section_nr_to_pfn() always clears PFN_SECTION_SHIFT
 *      lowest bits.  PFN_SECTION_SHIFT is arch-specific
 *      (equal SECTION_SIZE_BITS - PAGE_SHIFT), and the
 *      worst combination is powerpc with 256k pages,
 *      which results in PFN_SECTION_SHIFT equal 6.
 * To sum it up, at least 6 bits are available.
 */
#define	SECTION_MARKED_PRESENT	(1UL<<0)
#define SECTION_HAS_MEM_MAP	(1UL<<1)
#define SECTION_IS_ONLINE	(1UL<<2)
#define SECTION_IS_EARLY	(1UL<<3)
#define SECTION_MAP_LAST_BIT	(1UL<<4)
#define SECTION_MAP_MASK	(~(SECTION_MAP_LAST_BIT-1))
#define SECTION_NID_SHIFT	3

static inline struct page *__section_mem_map_addr(struct mem_section *section)
{
	unsigned long map = section->section_mem_map;
	map &= SECTION_MAP_MASK;
	return (struct page *)map;
}

static inline int present_section(struct mem_section *section)
{
	return (section && (section->section_mem_map & SECTION_MARKED_PRESENT));
}

static inline int present_section_nr(unsigned long nr)
{
	return present_section(__nr_to_section(nr));
}

static inline int valid_section(struct mem_section *section)
{
	return (section && (section->section_mem_map & SECTION_HAS_MEM_MAP));
}

static inline int early_section(struct mem_section *section)
{
	return (section && (section->section_mem_map & SECTION_IS_EARLY));
}

static inline int valid_section_nr(unsigned long nr)
{
	return valid_section(__nr_to_section(nr));
}

static inline int online_section(struct mem_section *section)
{
	return (section && (section->section_mem_map & SECTION_IS_ONLINE));
}

static inline int online_section_nr(unsigned long nr)
{
	return online_section(__nr_to_section(nr));
}

#ifdef CONFIG_MEMORY_HOTPLUG
void online_mem_sections(unsigned long start_pfn, unsigned long end_pfn);
#ifdef CONFIG_MEMORY_HOTREMOVE
void offline_mem_sections(unsigned long start_pfn, unsigned long end_pfn);
#endif
#endif

static inline struct mem_section *__pfn_to_section(unsigned long pfn)
{
	return __nr_to_section(pfn_to_section_nr(pfn));
}

extern unsigned long __highest_present_section_nr;

static inline int subsection_map_index(unsigned long pfn)
{
	return (pfn & ~(PAGE_SECTION_MASK)) / PAGES_PER_SUBSECTION;
}

#ifdef CONFIG_SPARSEMEM_VMEMMAP
static inline int pfn_section_valid(struct mem_section *ms, unsigned long pfn)
{
	int idx = subsection_map_index(pfn);

	return test_bit(idx, ms->usage->subsection_map);
}
#else
static inline int pfn_section_valid(struct mem_section *ms, unsigned long pfn)
{
	return 1;
}
#endif

#ifndef CONFIG_HAVE_ARCH_PFN_VALID
static inline int pfn_valid(unsigned long pfn)
{
	struct mem_section *ms;

	if (pfn_to_section_nr(pfn) >= NR_MEM_SECTIONS)
		return 0;
	ms = __nr_to_section(pfn_to_section_nr(pfn));
	if (!valid_section(ms))
		return 0;
	/*
	 * Traditionally early sections always returned pfn_valid() for
	 * the entire section-sized span.
	 */
	return early_section(ms) || pfn_section_valid(ms, pfn);
}
#endif

static inline int pfn_in_present_section(unsigned long pfn)
{
	if (pfn_to_section_nr(pfn) >= NR_MEM_SECTIONS)
		return 0;
	return present_section(__nr_to_section(pfn_to_section_nr(pfn)));
}

static inline unsigned long next_present_section_nr(unsigned long section_nr)
{
	while (++section_nr <= __highest_present_section_nr) {
		if (present_section_nr(section_nr))
			return section_nr;
	}

	return -1;
}

/*
 * These are _only_ used during initialisation, therefore they
 * can use __initdata ...  They could have names to indicate
 * this restriction.
 */
#ifdef CONFIG_NUMA
#define pfn_to_nid(pfn)							\
({									\
	unsigned long __pfn_to_nid_pfn = (pfn);				\
	page_to_nid(pfn_to_page(__pfn_to_nid_pfn));			\
})
#else
#define pfn_to_nid(pfn)		(0)
#endif

#define early_pfn_valid(pfn)	pfn_valid(pfn)
void sparse_init(void);
#else
#define sparse_init()	do {} while (0)
#define sparse_index_init(_sec, _nid)  do {} while (0)
#define pfn_in_present_section pfn_valid
#define subsection_map_init(_pfn, _nr_pages) do {} while (0)
#endif /* CONFIG_SPARSEMEM */

/*
 * During memory init memblocks map pfns to nids. The search is expensive and
 * this caches recent lookups. The implementation of __early_pfn_to_nid
 * may treat start/end as pfns or sections.
 */
struct mminit_pfnnid_cache {
	unsigned long last_start;
	unsigned long last_end;
	int last_nid;
};

#ifndef early_pfn_valid
#define early_pfn_valid(pfn)	(1)
#endif

void memory_present(int nid, unsigned long start, unsigned long end);

/*
 * If it is possible to have holes within a MAX_ORDER_NR_PAGES, then we
 * need to check pfn validity within that MAX_ORDER_NR_PAGES block.
 * pfn_valid_within() should be used in this case; we optimise this away
 * when we have no holes within a MAX_ORDER_NR_PAGES block.
 */
#ifdef CONFIG_HOLES_IN_ZONE
#define pfn_valid_within(pfn) pfn_valid(pfn)
#else
#define pfn_valid_within(pfn) (1)
#endif

#ifdef CONFIG_ARCH_HAS_HOLES_MEMORYMODEL
/*
 * pfn_valid() is meant to be able to tell if a given PFN has valid memmap
 * associated with it or not. This means that a struct page exists for this
 * pfn. The caller cannot assume the page is fully initialized in general.
 * Hotplugable pages might not have been onlined yet. pfn_to_online_page()
 * will ensure the struct page is fully online and initialized. Special pages
 * (e.g. ZONE_DEVICE) are never onlined and should be treated accordingly.
 *
 * In FLATMEM, it is expected that holes always have valid memmap as long as
 * there is valid PFNs either side of the hole. In SPARSEMEM, it is assumed
 * that a valid section has a memmap for the entire section.
 *
 * However, an ARM, and maybe other embedded architectures in the future
 * free memmap backing holes to save memory on the assumption the memmap is
 * never used. The page_zone linkages are then broken even though pfn_valid()
 * returns true. A walker of the full memmap must then do this additional
 * check to ensure the memmap they are looking at is sane by making sure
 * the zone and PFN linkages are still valid. This is expensive, but walkers
 * of the full memmap are extremely rare.
 */
bool memmap_valid_within(unsigned long pfn,
					struct page *page, struct zone *zone);
#else
static inline bool memmap_valid_within(unsigned long pfn,
					struct page *page, struct zone *zone)
{
	return true;
}
#endif /* CONFIG_ARCH_HAS_HOLES_MEMORYMODEL */

#endif /* !__GENERATING_BOUNDS.H */
#endif /* !__ASSEMBLY__ */
#endif /* _LINUX_MMZONE_H */
