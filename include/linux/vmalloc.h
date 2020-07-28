/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_VMALLOC_H
#define _LINUX_VMALLOC_H

#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/llist.h>
#include <asm/page.h>		/* pgprot_t */
#include <linux/rbtree.h>
#include <linux/overflow.h>

#include <asm/vmalloc.h>

struct vm_area_struct;		/* vma defining user mapping in mm_types.h */
struct notifier_block;		/* in notifier.h */

/* bits in flags of vmalloc's vm_struct below */
#define VM_IOREMAP		0x00000001	/* ioremap() and friends */
#define VM_ALLOC		0x00000002	/* vmalloc() */
#define VM_MAP			0x00000004	/* vmap()ed pages */
#define VM_USERMAP		0x00000008	/* suitable for remap_vmalloc_range */
#define VM_DMA_COHERENT		0x00000010	/* dma_alloc_coherent */
#define VM_UNINITIALIZED	0x00000020	/* vm_struct is not fully initialized */
#define VM_NO_GUARD		0x00000040      /* don't add guard page */
#define VM_KASAN		0x00000080      /* has allocated kasan shadow memory */

/*
 * VM_KASAN is used slighly differently depending on CONFIG_KASAN_VMALLOC.
 *
 * If IS_ENABLED(CONFIG_KASAN_VMALLOC), VM_KASAN is set on a vm_struct after
 * shadow memory has been mapped. It's used to handle allocation errors so that
 * we don't try to poision shadow on free if it was never allocated.
 *
 * Otherwise, VM_KASAN is set for kasan_module_alloc() allocations and used to
 * determine which allocations need the module shadow freed.
 */

/*
 * Memory with VM_FLUSH_RESET_PERMS cannot be freed in an interrupt or with
 * vfree_atomic().
 */
#define VM_FLUSH_RESET_PERMS	0x00000100      /* Reset direct map and flush TLB on unmap */

/* bits [20..32] reserved for arch specific ioremap internals */

/*
 * Maximum alignment for ioremap() regions.
 * Can be overriden by arch-specific value.
 */
#ifndef IOREMAP_MAX_ORDER
#define IOREMAP_MAX_ORDER	(7 + PAGE_SHIFT)	/* 128 pages */
#endif

struct vm_struct {
/** comment by hy 2018-01-06
 * # 下一个vm_struct实例
 */
	struct vm_struct	*next;
/** comment by hy 2018-01-06
 * # 其实虚拟地址
 */
	void			*addr;
/** comment by hy 2018-01-06
 * # 长度
 */
	unsigned long		size;
/** comment by hy 2018-01-06
 * # 标志位
 */
	unsigned long		flags;
/** comment by hy 2018-01-06
 * # 内存页
 */
	struct page		**pages;
/** comment by hy 2018-01-06
 * # 页数
 */
	unsigned int		nr_pages;
/** comment by hy 2018-01-06
 * # 其实物理地址
 */
	phys_addr_t		phys_addr;
	const void		*caller;
};

/** comment by hy 2018-01-06
 * #  每个虚拟内存区域对应一个 vmap_area
        [va_start, va_end)
 */
struct vmap_area {
/** comment by hy 2018-01-06
 * # 起始虚拟地址
 */
	unsigned long va_start;
/** comment by hy 2018-01-06
 * # 结束虚拟地址
 */
	unsigned long va_end;
/** comment by hy 2018-01-06
 * # 红黑树
        借助红黑树可以通过虚拟地址快速的查找到vmap_area
 */
	struct rb_node rb_node;         /* address sorted rbtree */
	struct list_head list;          /* address sorted list */
/** comment by hy 2018-01-06
 * # 指向 vm_struct 实例
 */
	/*
	 * The following three variables can be packed, because
	 * a vmap_area object is always one of the three states:
	 *    1) in "free" tree (root is vmap_area_root)
	 *    2) in "busy" tree (root is free_vmap_area_root)
	 *    3) in purge list  (head is vmap_purge_list)
	 */
	union {
		unsigned long subtree_max_size; /* in "free" tree */
		struct vm_struct *vm;           /* in "busy" tree */
		struct llist_node purge_list;   /* in purge list */
	};
};

/*
 *	Highlevel APIs for driver use
 */
extern void vm_unmap_ram(const void *mem, unsigned int count);
extern void *vm_map_ram(struct page **pages, unsigned int count, int node);
extern void vm_unmap_aliases(void);

#ifdef CONFIG_MMU
extern void __init vmalloc_init(void);
extern unsigned long vmalloc_nr_pages(void);
#else
static inline void vmalloc_init(void)
{
}
static inline unsigned long vmalloc_nr_pages(void) { return 0; }
#endif

extern void *vmalloc(unsigned long size);
extern void *vzalloc(unsigned long size);
extern void *vmalloc_user(unsigned long size);
extern void *vmalloc_node(unsigned long size, int node);
extern void *vzalloc_node(unsigned long size, int node);
extern void *vmalloc_32(unsigned long size);
extern void *vmalloc_32_user(unsigned long size);
extern void *__vmalloc(unsigned long size, gfp_t gfp_mask);
extern void *__vmalloc_node_range(unsigned long size, unsigned long align,
			unsigned long start, unsigned long end, gfp_t gfp_mask,
			pgprot_t prot, unsigned long vm_flags, int node,
			const void *caller);
void *__vmalloc_node(unsigned long size, unsigned long align, gfp_t gfp_mask,
		int node, const void *caller);

extern void vfree(const void *addr);
extern void vfree_atomic(const void *addr);

extern void *vmap(struct page **pages, unsigned int count,
			unsigned long flags, pgprot_t prot);
extern void vunmap(const void *addr);

extern int remap_vmalloc_range_partial(struct vm_area_struct *vma,
				       unsigned long uaddr, void *kaddr,
				       unsigned long pgoff, unsigned long size);

extern int remap_vmalloc_range(struct vm_area_struct *vma, void *addr,
							unsigned long pgoff);

/*
 * Architectures can set this mask to a combination of PGTBL_P?D_MODIFIED values
 * and let generic vmalloc and ioremap code know when arch_sync_kernel_mappings()
 * needs to be called.
 */
#ifndef ARCH_PAGE_TABLE_SYNC_MASK
#define ARCH_PAGE_TABLE_SYNC_MASK 0
#endif

/*
 * There is no default implementation for arch_sync_kernel_mappings(). It is
 * relied upon the compiler to optimize calls out if ARCH_PAGE_TABLE_SYNC_MASK
 * is 0.
 */
void arch_sync_kernel_mappings(unsigned long start, unsigned long end);

/*
 *	Lowlevel-APIs (not for driver use!)
 */

static inline size_t get_vm_area_size(const struct vm_struct *area)
{
	if (!(area->flags & VM_NO_GUARD))
		/* return actual size without guard page */
		return area->size - PAGE_SIZE;
	else
		return area->size;

}

extern struct vm_struct *get_vm_area(unsigned long size, unsigned long flags);
extern struct vm_struct *get_vm_area_caller(unsigned long size,
					unsigned long flags, const void *caller);
extern struct vm_struct *__get_vm_area_caller(unsigned long size,
					unsigned long flags,
					unsigned long start, unsigned long end,
					const void *caller);
extern struct vm_struct *remove_vm_area(const void *addr);
extern struct vm_struct *find_vm_area(const void *addr);

#ifdef CONFIG_MMU
extern int map_kernel_range_noflush(unsigned long start, unsigned long size,
				    pgprot_t prot, struct page **pages);
int map_kernel_range(unsigned long start, unsigned long size, pgprot_t prot,
		struct page **pages);
extern void unmap_kernel_range_noflush(unsigned long addr, unsigned long size);
extern void unmap_kernel_range(unsigned long addr, unsigned long size);
static inline void set_vm_flush_reset_perms(void *addr)
{
	struct vm_struct *vm = find_vm_area(addr);

	if (vm)
		vm->flags |= VM_FLUSH_RESET_PERMS;
}
#else
static inline int
map_kernel_range_noflush(unsigned long start, unsigned long size,
			pgprot_t prot, struct page **pages)
{
	return size >> PAGE_SHIFT;
}
#define map_kernel_range map_kernel_range_noflush
static inline void
unmap_kernel_range_noflush(unsigned long addr, unsigned long size)
{
}
#define unmap_kernel_range unmap_kernel_range_noflush
static inline void set_vm_flush_reset_perms(void *addr)
{
}
#endif

/* Allocate/destroy a 'vmalloc' VM area. */
extern struct vm_struct *alloc_vm_area(size_t size, pte_t **ptes);
extern void free_vm_area(struct vm_struct *area);

/* for /dev/kmem */
extern long vread(char *buf, char *addr, unsigned long count);
extern long vwrite(char *buf, char *addr, unsigned long count);

/*
 *	Internals.  Dont't use..
 */
extern struct list_head vmap_area_list;
extern __init void vm_area_add_early(struct vm_struct *vm);
extern __init void vm_area_register_early(struct vm_struct *vm, size_t align);

#ifdef CONFIG_SMP
# ifdef CONFIG_MMU
struct vm_struct **pcpu_get_vm_areas(const unsigned long *offsets,
				     const size_t *sizes, int nr_vms,
				     size_t align);

void pcpu_free_vm_areas(struct vm_struct **vms, int nr_vms);
# else
static inline struct vm_struct **
pcpu_get_vm_areas(const unsigned long *offsets,
		const size_t *sizes, int nr_vms,
		size_t align)
{
	return NULL;
}

static inline void
pcpu_free_vm_areas(struct vm_struct **vms, int nr_vms)
{
}
# endif
#endif

#ifdef CONFIG_MMU
#define VMALLOC_TOTAL (VMALLOC_END - VMALLOC_START)
#else
#define VMALLOC_TOTAL 0UL
#endif

int register_vmap_purge_notifier(struct notifier_block *nb);
int unregister_vmap_purge_notifier(struct notifier_block *nb);

#endif /* _LINUX_VMALLOC_H */
