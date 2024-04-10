/*
 * Copyright (c) 2024 Free Software Foundation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "aarch64/pmap.h"
#include "aarch64/vm_param.h"
#include "aarch64/hwcaps.h"
#include "aarch64/bits/pte.h"
#include "aarch64/bits/mair.h"
#include "aarch64/bits/tcr.h"
#include "aarch64/bits/sctlr.h"
#include <vm/pmap.h>
#include <vm/vm_page.h>
#include <vm/vm_kern.h>
#include <kern/slab.h>
#include <kern/printf.h>
#include <device/dtb.h>
#include <string.h>

struct pmap {
	pt_entry_t	*l0_base;	/* TTBR0 */
	unsigned short	asid;
	int		ref_count;
	decl_simple_lock_data(,lock)	/* lock on map */
	struct pmap_statistics stats;	/* map statistics */
};

#define TTBR_ASID(x)		(((x) >> 48) & 0xff)
#define TTBR_MAKE_ASID(asid)	(((vm_offset_t) asid) << 48)
#define TTBR_ASID_MASK		0xffff000000000000UL
#define TTBR_BADDR_MASK		0x0000fffffffffffeUL	/* translation table base address */

static boolean_t	pmap_initialized = FALSE;

static struct pmap	kernel_pmap_store;
pmap_t			kernel_pmap;
static pt_entry_t	*ttbr1_l0_base;

static struct kmem_cache pmap_cache;
static struct kmem_cache table_cache;

typedef struct pv_entry {
	struct pv_entry	*next;
	pmap_t		pmap;
	vm_offset_t	va;
} *pv_entry_t;

#define PV_ENTRY_NULL	((pv_entry_t) 0)

static pv_entry_t	pv_head_table;
static pv_entry_t	pv_free_list;
def_simple_lock_data(static, pv_free_list_lock)

static pv_entry_t pv_alloc(void)
{
	pv_entry_t	pv_e;

	simple_lock(&pv_free_list_lock);
	pv_e = pv_free_list;
	if (pv_e != PV_ENTRY_NULL)
		pv_free_list = pv_free_list->next;
	simple_unlock(&pv_free_list_lock);

	return pv_e;
}

static void pv_free(pv_entry_t pv_e)
{
	simple_lock(&pv_free_list_lock);
	pv_e->next = pv_free_list;
	pv_free_list = pv_e->next;
	simple_unlock(&pv_free_list_lock);
}

static struct kmem_cache pv_list_cache;



#if NCPUS > 1

#error "Implement"

static lock_data_t	pmap_system_lock;

#define PMAP_READ_LOCK(pmap, spl)					\
MACRO_BEGIN								\
	SPLVM(spl);							\
	lock_read(&pmap_system_lock);					\
	simple_lock(&(pmap)->lock);					\
MACRO_END

#define PMAP_READ_UNLOCK(pmap, spl)					\
MACRO_BEGIN								\
	simple_unlock(&(pmap)->lock);					\
	lock_read_done(&pmap_system_lock);				\
	SPLX(spl);							\
MACRO_END

#define PMAP_WRITE_LOCK(spl)						\
MACRO_BEGIN								\
	SPLVM(spl);							\
	lock_write(&pmap_system_lock);					\
MACRO_END

#define PMAP_WRITE_UNLOCK(spl)						\
MACRO_BEGIN								\
	lock_write_done(&pmap_system_lock);				\
	SPLX(spl);							\
NACRO_END

#else

#define SPLVM(spl)	(spl) = 0
#define SPLX(spl)	(void) (spl)

#define PMAP_READ_LOCK(pmap, spl)	SPLVM(spl)
#define PMAP_READ_UNLOCK(pmap, spl)	SPLX(spl)
#define PMAP_WRITE_LOCK(spl)		SPLVM(spl)
#define PMAP_WRITE_UNLOCK(spl)		SPLX(spl)

#endif


/*
 *	Range of kernel virtual addresses available for kernel memory mapping.
 *	Does not include the virtual addresses used to map physical memory 1-1.
 *	Initialized by pmap_bootstrap.
 */
vm_offset_t		kernel_virtual_start;
vm_offset_t		kernel_virtual_end;

/*
 *	The (single largest) region of physical memory.
 */
static phys_addr_t	phys_mem_start;
static vm_size_t	phys_mem_size;

extern const void	__text_start;
extern const void	_image_end;

/*
 * Two slots for temporary physical page mapping, to allow for
 * physical-to-physical transfers.
 */
#define MAPWINDOW_SIZE (PMAP_NMAPWINDOWS * NCPUS * PAGE_SIZE)
static void pmap_init_mapwindows(void);

static boolean_t valid_page(phys_addr_t addr)
{
	if (!pmap_initialized)
		return FALSE;
	return vm_page_lookup_pa(addr) != VM_PAGE_NULL;
}

#define pa_index(pa)		vm_page_table_index(pa)
#define pai_to_pvh(pai)		(&pv_head_table[pai])

/*
 *	Early physical memory heap.
 */
static vm_offset_t heap_start;

vm_offset_t pmap_grab_page(void)
{
	vm_offset_t res = heap_start;

	heap_start += PAGE_SIZE;

	return res;
}

static void __attribute__((noinline)) pmap_ungrab_page(vm_offset_t page)
{
	if (page + PAGE_SIZE == heap_start)
		heap_start = page;
}


static inline void cache_flush(void)
{
	asm volatile(
		"dsb st\n\t"
		"isb sy"
		::: "memory");
}

/* TODO: callers of this should pass VAE1IS, once we mark memory as shareable */
#define TLB_FLUSH(kind, arg)						\
MACRO_BEGIN								\
	asm volatile("dsb ishst");					\
	asm volatile("tlbi " kind ", %0" :: "r"(arg));			\
MACRO_END

void pmap_discover_physical_memory(dtb_node_t node)
{
	struct dtb_prop	prop;
	dtb_t		dtb;
	phys_addr_t	start, kernel_start;
	vm_size_t	size, dtb_size;
	vm_size_t	off = 0;

	prop = dtb_node_find_prop(node, "reg");
	assert(!DTB_IS_SENTINEL(prop));

	/*
	 *	TODO: We currently only consider a single largest
	 *	region of memory.  It appears to be a limitation
	 *	of the vm_page module, it can only handle a single
	 *	region at the given "seg_index", of which there are
	 *	only 4?
	 */

	while (off < prop.length) {
		start = dtb_prop_read_cells(&prop,
			node->address_cells,
			&off);
		size = dtb_prop_read_cells(&prop,
			node->size_cells,
			&off);

		if (size > phys_mem_size) {
			phys_mem_start = start;
			phys_mem_size = size;
		}
	}

	assert(phys_mem_size > 0);
	/* TODO: is VM_PAGE_SEG_DMA appropriate here? */
	vm_page_load(VM_PAGE_SEG_DMA,
		phys_mem_start,
		phys_mem_start + phys_mem_size);

	/*
	 *	If the kernel itself or the DTB is loaded in
	 *	this region of memory, exclude them form the heap.
	 */
	kernel_start = (phys_addr_t) &__text_start;
	heap_start = phys_mem_start;
	if (kernel_start >= heap_start
	    && kernel_start < phys_mem_start + phys_mem_size) {
		heap_start = round_page((phys_addr_t) &_image_end);
	}
	dtb_get_location(&dtb, &dtb_size);
	if ((phys_addr_t) dtb >= heap_start
	    && (phys_addr_t) dtb < phys_mem_start + phys_mem_size) {
		heap_start = round_page(((phys_addr_t) dtb) + dtb_size);
	}
}

_Static_assert((1UL << VM_AARCH64_T0SZ) == VM_MAX_USER_ADDRESS);
_Static_assert((1UL << VM_AARCH64_T1SZ) + VM_MIN_KERNEL_ADDRESS == 0UL);

#define BITS_PER_LEVEL		9	/* 4K granularity */
#define NEXT_SB(sb)		(((sb) - PAGE_SHIFT - 1) / BITS_PER_LEVEL * BITS_PER_LEVEL + PAGE_SHIFT)
#define TT_INDEX(v, sb, nsb)	(((v) >> (nsb)) & ((1UL << ((sb) - (nsb))) - 1))

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	Allocate the kernel page translation tables,
 *	and direct-map all physical memory.
 *	Called with mapping off.
 */
void __attribute__((target("branch-protection=none"))) pmap_bootstrap(void)
{
#if (VM_AARCH64_T0SZ != 48) || (VM_AARCH64_T1SZ != 48)
#error "Rework the logic below"
#endif
	phys_addr_t	kernel_block_t0_l1;
	unsigned long	kernel_block_t0_l1_index;
	phys_addr_t	kernel_block_t1_l1;
	unsigned long	kernel_block_t1_l1_index;

	uint64_t	sctlr;
	uintptr_t	scratch1, scratch2;
	pt_entry_t	*phys_ttbr0_l0_base;
	pt_entry_t	*phys_ttbr1_l0_base;
	pt_entry_t	*phys_ttbr0_l1_base;
	pt_entry_t	*phys_ttbr1_l1_base;
	pt_entry_t	kernel_mapping_bti;

	/*
	 *	When the kernel itself is compiled with BTI
	 *	(enabled with -mbranch-protection=bti in GCC),
	 *	enable BTI enforcement for the kernel mapping.
	 */
#ifdef __ARM_FEATURE_BTI_DEFAULT
	kernel_mapping_bti = AARCH64_PTE_BTI;
#else
	kernel_mapping_bti = 0;
#endif

	kernel_block_t0_l1_index = TT_INDEX((phys_addr_t) &__text_start, 36, NEXT_SB(36));
	kernel_block_t0_l1 = kernel_block_t0_l1_index << NEXT_SB(36);
	kernel_block_t1_l1_index = TT_INDEX((phys_addr_t) &__text_start, 36, NEXT_SB(36));
	kernel_block_t1_l1 = kernel_block_t1_l1_index << NEXT_SB(36);

	phys_ttbr1_l0_base = (pt_entry_t*)pmap_grab_page();
	phys_ttbr1_l1_base = (pt_entry_t*)pmap_grab_page();

	ttbr1_l0_base = (pt_entry_t*)phystokv(phys_ttbr1_l0_base);

	/* Make sure to grab these ones last, so they can be then released.  */
	phys_ttbr0_l0_base = (pt_entry_t*)pmap_grab_page();
	phys_ttbr0_l1_base = (pt_entry_t*)pmap_grab_page();

	memset(phys_ttbr0_l0_base, 0, PAGE_SIZE);
	memset(phys_ttbr0_l1_base, 0, PAGE_SIZE);
	/* Temporary identity map.  */
	phys_ttbr0_l0_base[0] = (phys_addr_t) phys_ttbr0_l1_base
		| AARCH64_PTE_MAIR_INDEX(MAIR_NORMAL_INDEX)
		| AARCH64_PTE_ACCESS
		| AARCH64_PTE_TABLE
		| AARCH64_PTE_VALID
		| AARCH64_PTE_UXN
		| kernel_mapping_bti
		| AARCH64_PTE_NON_SH /* ? */;
	phys_ttbr0_l1_base[kernel_block_t0_l1_index] = kernel_block_t0_l1
		| AARCH64_PTE_MAIR_INDEX(MAIR_NORMAL_INDEX)
		| AARCH64_PTE_ACCESS
		| AARCH64_PTE_BLOCK
		| AARCH64_PTE_VALID
		| AARCH64_PTE_UXN
		| kernel_mapping_bti
		| AARCH64_PTE_NON_SH /* ? */;

	/* The upper map.  */
	memset(phys_ttbr1_l0_base, 0, PAGE_SIZE);
	memset(phys_ttbr1_l1_base, 0, PAGE_SIZE);
	/* TODO: This assumes that physical memory is in the first block.  */
	phys_ttbr1_l0_base[0] = (phys_addr_t) phys_ttbr1_l1_base
		| AARCH64_PTE_MAIR_INDEX(MAIR_NORMAL_INDEX)
		| AARCH64_PTE_ACCESS
		| AARCH64_PTE_TABLE
		| AARCH64_PTE_VALID
		| AARCH64_PTE_UXN
		| AARCH64_PTE_NON_SH /* ? */;
	phys_ttbr1_l1_base[0] = 0x0
		| AARCH64_PTE_MAIR_INDEX(MAIR_NORMAL_INDEX)
		| AARCH64_PTE_ACCESS
		| AARCH64_PTE_BLOCK
		| AARCH64_PTE_VALID
		| AARCH64_PTE_UXN
		| AARCH64_PTE_PXN
		| AARCH64_PTE_NON_SH /* ? */;
	/* FIXME: MAIR device index... */
	phys_ttbr1_l1_base[kernel_block_t1_l1_index] = kernel_block_t1_l1
		| AARCH64_PTE_MAIR_INDEX(MAIR_NORMAL_INDEX)
		| AARCH64_PTE_ACCESS
		| AARCH64_PTE_BLOCK
		| AARCH64_PTE_VALID
		| AARCH64_PTE_UXN
		| kernel_mapping_bti
		| AARCH64_PTE_NON_SH /* ? */;

	/*
	 *	Determine the kernel virtual address range.
	 *	It starts at the end of the physical memory
	 *	mapped into the kernel address space,
	 *	and extends to a stupid arbitrary limit beyond that.
	 */
	kernel_virtual_start = phystokv(kernel_block_t1_l1 + (1UL << NEXT_SB(36)));
	kernel_virtual_end = kernel_virtual_start + VM_KERNEL_MAP_SIZE;

	sctlr = SCTLR_M | SCTLR_SA | SCTLR_SA0 | SCTLR_UCT | SCTLR_UCI
		| SCTLR_BT0 | SCTLR_BT1;
	if (hwcap_internal & HWCAP_INT_EPAN)
		sctlr |= SCTLR_EPAN;
	if (hwcaps[0] & HWCAP_SSBS)
		sctlr |= SCTLR_SSBS;

	/* Enable the MMU.  */
	asm volatile("msr MAIR_EL1, %0" :: "r"(MAIR_VALUE));
	asm volatile("msr TCR_EL1, %0" :: "r"(TCR_VALUE));
	asm volatile("msr TTBR0_EL1, %0" :: "r"(phys_ttbr0_l0_base));
	asm volatile("msr TTBR1_EL1, %0" :: "r"(phys_ttbr1_l0_base));
	asm volatile(
		"isb sy\n\t"
		"msr SCTLR_EL1, %[sctlr]\n\t"
		"dsb st\n\t"
		"isb sy\n\t"
		/* Adjust sp, x29 to high memory.  */
		"mov %[scratch1], #%[min_addr]\n\t"
		"add sp, sp, %[scratch1]\n\t"
		"add x29, x29, %[scratch1]\n\t"
		/* Jump to high memory.  */
		"adr %[scratch2], .here\n\t"
		"add %[scratch2], %[scratch2], %[scratch1]\n\t"
		"br %[scratch2]\n"
		".here:\n\t"
#ifdef __ARM_FEATURE_BTI_DEFAULT
		"bti j\n\t"
#endif
		/* Now adjust the saved x29 / x30.  */
		"ldp %[scratch2], x30, [x29]\n\t"
		"add %[scratch2], %[scratch2], %[scratch1]\n\t"
		"add x30, x30, %[scratch1]\n\t"
		"stp %[scratch2], x30, [x29]\n\t"
		/*
		 *	Unload the identity mapping, just to make
		 *	sure we crash if anything still references it.
		 */
		"mov %[scratch2], #-1\n\t"
		"msr TTBR0_EL1, %[scratch2]"
		:
		[scratch1] "=&r"(scratch1),
		[scratch2] "=&r"(scratch2)
		:
		[sctlr] "r"(sctlr),
		[min_addr] "i"(VM_MIN_KERNEL_ADDRESS)
		: "memory"
	);

	/*
	 *	Release the pages used for the identity mapping,
	 *	since we no longer need them.  This must be done
	 *	in the reverse order compared to how we grabbed
	 *	them above.
	 */
	pmap_ungrab_page((vm_offset_t) phys_ttbr0_l1_base);
	pmap_ungrab_page((vm_offset_t) phys_ttbr0_l0_base);
}

void pmap_bootstrap_misc(void)
{
	extern boolean_t	vm_fault_dirty_handling;

	kernel_pmap = &kernel_pmap_store;
#if NCPUS > 1
	lock_init(&pmap_system_lock, FALSE);
#endif
	simple_lock_init(&kernel_pmap->lock);
	kernel_pmap->ref_count = 1;
	kernel_pmap->l0_base = PT_ENTRY_NULL;
	kernel_pmap->asid = 0;

	vm_page_load_heap(VM_PAGE_SEG_DMA, heap_start, phys_mem_start + phys_mem_size);
	pmap_init_mapwindows();

	vm_fault_dirty_handling = TRUE;
}

void pmap_virtual_space(
	vm_offset_t *startp,
	vm_offset_t *endp)
{
	*startp = kernel_virtual_start;
	*endp = kernel_virtual_end - MAPWINDOW_SIZE;
}

void pmap_activate_user(pmap_t p)
{
	vm_offset_t	ttbr0, asid_hi;

	asid_hi = TTBR_MAKE_ASID(1);
	ttbr0 = kvtophys(p->l0_base) | asid_hi;
	asm volatile("msr TTBR0_EL1, %0" :: "r"(ttbr0));
	TLB_FLUSH("aside1", asid_hi);
	cache_flush();
}

void pmap_init(void)
{
	kern_return_t	kr;
	unsigned long	npages;
	vm_offset_t	addr;

	npages = vm_page_table_size();

	kr = kmem_alloc_wired(kernel_map, &addr, sizeof(struct pv_entry) * npages);
	assert(kr == KERN_SUCCESS);
	pv_head_table = (pv_entry_t) addr;

	kmem_cache_init(&pmap_cache, "pmap", sizeof(struct pmap), alignof(struct pmap), NULL, 0);
	kmem_cache_init(&table_cache, "page table", PAGE_SIZE, PAGE_SIZE, NULL, KMEM_CACHE_PHYSMEM);
	kmem_cache_init(&pv_list_cache, "pv_entry", sizeof(struct pv_entry), alignof(struct pv_entry), NULL, 0);

	pmap_initialized = TRUE;
}

#ifdef notyet
static unsigned short next_asid;
#endif

pmap_t pmap_create(vm_size_t size)
{
	pmap_t	p;

	if (size != 0)
		return PMAP_NULL;

	p = (pmap_t) kmem_cache_alloc(&pmap_cache);
	if (p == PMAP_NULL)
		return PMAP_NULL;

	p->l0_base = (pt_entry_t*)kmem_cache_alloc(&table_cache);
	if (p->l0_base == PT_ENTRY_NULL) {
		kmem_cache_free(&pmap_cache, (vm_offset_t)p);
		return PMAP_NULL;
	}
	memset(p->l0_base, 0, PAGE_SIZE);

#ifdef notyet
#if NCPUS > 1
#error "Need some synch here"
#endif
	p->asid = ++next_asid;
	if (likely(hwcap_internal & HWCAP_INT_ASID16))
		p->asid &= 0xffff;
	else
		p->asid &= 0xff;
#else
	p->asid = 1;
#endif

	p->ref_count = 1;
	simple_lock_init(&p->lock);

	p->stats.resident_count = 0;
	p->stats.wired_count = 0;

	return p;
}

integer_t pmap_resident_count(pmap_t pmap)
{
	return pmap->stats.resident_count;
}

void pmap_reference(pmap_t pmap)
{
	simple_lock(&pmap->lock);
	pmap->ref_count++;
	simple_unlock(&pmap->lock);
}

static void pmap_destroy_table(
	pt_entry_t	*table,
	unsigned char	significant_bits)
{
	unsigned	index, table_size;
	unsigned char	next_sb;
	boolean_t	last_level;
	pt_entry_t	entry;
	pt_entry_t	*next_table;

	assert(significant_bits > PAGE_SHIFT);
	next_sb = NEXT_SB(significant_bits);
	last_level = (next_sb == PAGE_SHIFT);
	table_size = 1U << (significant_bits - next_sb);

	if (!last_level) {
		for (index = 0; index < table_size; index++) {
			entry = table[index];
			if (!(entry & AARCH64_PTE_VALID)) {
				assert(entry == 0);
				continue;
			}
			if (entry & AARCH64_PTE_TABLE) {
				next_table = (pt_entry_t*)(entry & AARCH64_PTE_ADDR_MASK);
				assert(next_table != PT_ENTRY_NULL);
				next_table = (pt_entry_t*)phystokv(next_table);
				pmap_destroy_table(next_table, next_sb);
			}
		}
	}

	kmem_cache_free(&table_cache, (vm_offset_t) table);
}

void pmap_destroy(pmap_t pmap)
{
	int c;

	simple_lock(&pmap->lock);
	c = --pmap->ref_count;
	simple_unlock(&pmap->lock);

	if (c != 0)
		return;

	assert(pmap != kernel_pmap);
	assert(pmap->stats.wired_count == 0);

	pmap_destroy_table(pmap->l0_base, VM_AARCH64_T0SZ);
	kmem_cache_free(&pmap_cache, (vm_offset_t) pmap);
}

static pt_entry_t pmap_prot(vm_offset_t v, vm_prot_t prot)
{
	pt_entry_t	entry = 0;

	if (!(prot & VM_PROT_WRITE))
		entry |= AARCH64_PTE_READ_ONLY;

	if (v >= VM_MIN_KERNEL_ADDRESS) {
		assert(!(prot & VM_PROT_EXECUTE));
		entry |= AARCH64_PTE_PXN;
		entry |= AARCH64_PTE_UXN;
	} else {
		if (prot & (VM_PROT_READ | VM_PROT_WRITE))
			entry |= AARCH64_PTE_EL0_ACCESS;
		entry |= AARCH64_PTE_PXN;
		if (!(prot & VM_PROT_EXECUTE))
			entry |= AARCH64_PTE_UXN;
		/*
		 *	With EPAN, we can have truly execute-only
		 *	mappings in EL0.  But if EPAN is not available,
		 *	forcefully enable EL0 read access.
		 */
		else if (!(hwcap_internal & HWCAP_INT_EPAN))
			entry |= AARCH64_PTE_EL0_ACCESS;
	}

	return entry;
}

static void pmap_reduce_prot(
	pt_entry_t	*entry,
	pt_entry_t	prot_mask)
{
	*entry |= AARCH64_PTE_PXN;

	if (prot_mask & AARCH64_PTE_UXN)
		*entry |= AARCH64_PTE_UXN;
	if (prot_mask & AARCH64_PTE_READ_ONLY)
		*entry |= AARCH64_PTE_READ_ONLY;

	if (!(prot_mask & AARCH64_PTE_EL0_ACCESS))
		*entry &= ~AARCH64_PTE_EL0_ACCESS;

	/*
	 *	XXX: Convince myself that this cannot lead to
	 *	execute-only mappings unless we have EPAN.
	 */
}

static kern_return_t pmap_walk(
	pmap_t		pmap,
	pt_entry_t	*table,
	vm_offset_t	v,
	unsigned char	significant_bits,
	int		*spl,
	boolean_t	create,
	vm_prot_t	prot,
	pt_entry_t	**out_entry)
{
	unsigned	index;
	pt_entry_t	entry;
	unsigned char	next_sb;
	boolean_t	last_level;
	pt_entry_t	*next_table;

	assert(significant_bits > PAGE_SHIFT);
	next_sb = NEXT_SB(significant_bits);
	last_level = (next_sb == PAGE_SHIFT);

	index = TT_INDEX(v, significant_bits, next_sb);
	assert(index < PAGE_SIZE / sizeof(pt_entry_t));

Retry:
	entry = table[index];
	if (!(entry & AARCH64_PTE_VALID)) {
		assert(entry == 0);
		if (!create)
			return KERN_INVALID_ADDRESS;

		if (!last_level) {
			/*
			 *	This PTE will point to the next-level table.
			 *	Allocate and clear that now.  Unlock pmap
			 *	while trying to allocate.
			 */
			PMAP_READ_UNLOCK(pmap, *spl);
			if (!pmap_initialized) {
				next_table = (pt_entry_t*)phystokv(vm_page_bootalloc(PAGE_SIZE));
			} else {
				while (!(next_table = (pt_entry_t*)kmem_cache_alloc(&table_cache)))
					VM_PAGE_WAIT(0);
			}
			memset(next_table, 0, PAGE_SIZE);
			PMAP_READ_LOCK(pmap, *spl);
			entry = table[index];
			if (entry & AARCH64_PTE_VALID) {
				/*
				 *	Someone else has got to entering the
				 *	table before we did.  No big deal,
				 *	proceed as if nothing happened.  We
				 *	assume that concurrent pmap calls don't
				 *	otherwise conflict with each other, i.e.
				 *	they use appropriate locking at a higher
				 *	level (vm_map).
				 */
				PMAP_READ_UNLOCK(pmap, *spl);
				assert(pmap_initialized);
				kmem_cache_free(&table_cache, (vm_offset_t) next_table);
				PMAP_READ_LOCK(pmap, *spl);
				goto Retry;
			}
			entry = kvtophys(next_table) | AARCH64_PTE_TABLE;
		} else {
			/*
			 *	This PTE will point to a block; its address
			 *	will be filled in by our caller.
			 */
			entry = AARCH64_PTE_BLOCK | AARCH64_PTE_LEVEL3;
		}
		entry |= AARCH64_PTE_MAIR_INDEX(MAIR_NORMAL_INDEX)
			| AARCH64_PTE_ACCESS
			| AARCH64_PTE_VALID
			| AARCH64_PTE_NON_SH /* ?? */;
		if (v < VM_MAX_USER_ADDRESS)
			entry |= AARCH64_PTE_NG;

		entry |= pmap_prot(v, prot);
		table[index] = entry;
	}

	if (!(entry & AARCH64_PTE_TABLE) || last_level) {
		*out_entry = &table[index];
		return KERN_SUCCESS;
	}

	next_table = (pt_entry_t*)(entry & AARCH64_PTE_ADDR_MASK);
	assert(next_table != PT_ENTRY_NULL);
	next_table = (pt_entry_t*)phystokv(next_table);
	return pmap_walk(pmap, next_table, v, next_sb, spl, create, prot, out_entry);
}

static void pmap_walk_range(
	pt_entry_t	*table,
	vm_offset_t	sva,
	vm_offset_t	eva,
	unsigned char	significant_bits,
	void		(*callback)(vm_offset_t, pt_entry_t*, void*),
	void		*data)
{
	pt_entry_t	entry, *next_table;
	unsigned char	next_sb;
	unsigned	index;
	unsigned	table_size;
	boolean_t	last_level;
	vm_offset_t	v;

	assert(significant_bits > PAGE_SHIFT);
	next_sb = NEXT_SB(significant_bits);
	last_level = (next_sb == PAGE_SHIFT);
	index = TT_INDEX(sva, significant_bits, next_sb);
	table_size = 1U << (significant_bits - next_sb);
	v = sva;

	for (v = sva; v < eva && index < table_size; v += (1UL << next_sb), index++) {
		entry = table[index];
		if (!(entry & AARCH64_PTE_VALID)) {
			assert(entry == 0);
			callback(v, PT_ENTRY_NULL, data);
		} else if (!(entry & AARCH64_PTE_TABLE) || last_level) {
			callback(v, &table[index], data);
		} else {
			next_table = (pt_entry_t*)(entry & AARCH64_PTE_ADDR_MASK);
			assert(next_table != PT_ENTRY_NULL);
			next_table = (pt_entry_t*)phystokv(next_table);
			pmap_walk_range(next_table, v, eva, next_sb, callback, data);
		}
	}
}

static pt_entry_t *pmap_table(pmap_t pmap, vm_offset_t v)
{
	if (v >= VM_MIN_KERNEL_ADDRESS)
		return ttbr1_l0_base;
	assert(pmap != kernel_pmap);
	return pmap->l0_base;
}

static unsigned char pmap_root_sb(vm_offset_t v)
{
	if (v >= VM_MIN_KERNEL_ADDRESS)
		return VM_AARCH64_T1SZ;
	return VM_AARCH64_T0SZ;
}

static void pv_link(
	pmap_t		pmap,
	phys_addr_t	pa,
	vm_offset_t	v,
	spl_t		*spl)
{
	pv_entry_t	pv_e, pv_h;
	unsigned long	pai;

	if (!valid_page(pa))
		return;

	pv_e = PV_ENTRY_NULL;
Again:
	pai = pa_index(pa);
	// LOCK_PVH(pai);
	pv_h = pai_to_pvh(pai);
	if (pv_h->pmap == NULL) {
		pv_h->pmap = pmap;
		pv_h->va = v;
		pv_h->next = PV_ENTRY_NULL;
	} else {
		if (pv_e == PV_ENTRY_NULL)
			pv_e = pv_alloc();
		if (pv_e == PV_ENTRY_NULL) {
			// UNLOCK_VH(pai);
			PMAP_READ_UNLOCK(pmap, *spl);
			pv_e = (pv_entry_t) kmem_cache_alloc(&pv_list_cache);
			PMAP_READ_LOCK(pmap, *spl);
			goto Again;
		}
		pv_e->pmap = pmap;
		pv_e->va = v;
		pv_e->next = pv_h->next;
		pv_h->next = pv_e;
		pv_e = PV_ENTRY_NULL;
	}
	// UNLOCK_VH(pai);
	if (pv_e != PV_ENTRY_NULL)
		pv_free(pv_e);
}

static void pv_unlink(
	pmap_t		pmap,
	phys_addr_t	pa,
	vm_offset_t	v)
{
	pv_entry_t	pv_h;
	pv_entry_t	pv_e;
	pv_entry_t	pv_prev;
	unsigned long	pai;

	if (!valid_page(pa))
		return;

	pai = pa_index(pa);
	// LOCK_PVH(pai);
	pv_h = pai_to_pvh(pai);

	assert(pv_h->pmap != PMAP_NULL);
	if (pv_h->pmap == pmap && pv_h->va == v) {
		pv_e = pv_h->next;
		if (pv_e != PV_ENTRY_NULL) {
			*pv_h = *pv_e;
			pv_free(pv_e);
		} else {
			pv_h->pmap = PMAP_NULL;
			pv_h->va = 0;
		}
	} else {
		pv_e = pv_h;
		do {
			pv_prev = pv_e;
			assert(pv_e->next != PV_ENTRY_NULL);
			pv_e = pv_e->next;
		} while (pv_e->pmap != pmap || pv_e->va != v);
		pv_prev->next = pv_e->next;
		pv_free(pv_e);
	}
	// UNLOCK_PVH(pai);
}

/*
 *	Insert the given physical page (p) at
 *	the specified virtual address (v) in the
 *	target physical map with the protection requested.
 *
 *	If specified, the page will be wired down, meaning
 *	that the related pte can not be reclaimed.
 *
 *	NB:  This is the only routine which MAY NOT lazy-evaluate
 *	or lose information.  That is, this routine must actually
 *	insert this page into the given map NOW.
 */
void pmap_enter(
	pmap_t		pmap,
	vm_offset_t	v,
	phys_addr_t	pa,
	vm_prot_t	prot,
	boolean_t	wired)
{
	kern_return_t	kr;
	pt_entry_t	*entry;
	int		spl;
	phys_addr_t	prev_pa;

	assert(pmap != NULL);
	assert(pa != vm_page_fictitious_addr);

	if (pmap == kernel_pmap && (v < kernel_virtual_start || v >= kernel_virtual_end))
		panic("pmap_enter(%#016.lx, %#llx) falls in physical memory area!\n", (unsigned long) v, (unsigned long long) pa);

	if (pmap != kernel_pmap && v >= kernel_virtual_start)
		panic("pmap_enter(%#016.lx, %#llx) for a non-kernel pmap?\n", (unsigned long) v, (unsigned long long) pa);

	PMAP_READ_LOCK(pmap, spl);

	kr = pmap_walk(pmap, pmap_table(pmap, v), v, pmap_root_sb(v), &spl, TRUE, prot, &entry);
	assert(kr == KERN_SUCCESS);
	prev_pa = (*entry) & AARCH64_PTE_ADDR_MASK;
	*entry = ((*entry) & ~AARCH64_PTE_ADDR_MASK & ~AARCH64_PTE_PROT_MASK) | (pt_entry_t)pa | pmap_prot(v, prot);

	if (!prev_pa) {		/* FIXME what if it was pa 0x0 */
		pmap->stats.resident_count++;
		pv_link(pmap, pa, v, &spl);
	} else {
		assert(prev_pa == pa);
	}

	PMAP_READ_UNLOCK(pmap, spl);

	TLB_FLUSH("vae1", (v >> PAGE_SHIFT) | TTBR_MAKE_ASID(pmap->asid));
	cache_flush();
}


phys_addr_t pmap_extract(
	pmap_t		pmap,
	vm_offset_t	v)
{
	kern_return_t	kr;
	pt_entry_t	*entry;
	phys_addr_t	pa;
	int		spl;

	SPLVM(spl);
	simple_lock(&pmap->lock);

	kr = pmap_walk(pmap, pmap_table(pmap, v), v, pmap_root_sb(v), &spl, FALSE, VM_PROT_NONE, &entry);
	if (kr != KERN_SUCCESS)
		pa = 0;
	pa = (*entry) & AARCH64_PTE_ADDR_MASK;
	simple_unlock(&pmap->lock);
	SPLX(spl);
	return pa;
}

static void pmap_protect_callback(
	vm_offset_t	v,
	pt_entry_t	*entry,
	void		*data)
{
	pt_entry_t	prot = (pt_entry_t) data;

	if (entry == PT_ENTRY_NULL || !(*entry & AARCH64_PTE_VALID))
		return;

	pmap_reduce_prot(entry, prot);
}

void pmap_protect(
	pmap_t		pmap,
	vm_offset_t	sva,
	vm_offset_t	eva,
	vm_prot_t	prot)
{
	pt_entry_t	*table;
	vm_offset_t	v;
	int		spl;

	assert(sva < eva);

	SPLVM(spl);
	simple_lock(&pmap->lock);

	table = pmap_table(pmap, sva);
	assert(pmap_table(pmap, eva - 1) == table);

	pmap_walk_range(table, sva, eva, pmap_root_sb(sva), pmap_protect_callback, (void *) pmap_prot(sva, prot));

	simple_unlock(&pmap->lock);
	SPLX(spl);

	for (v = sva; v < eva; v += PAGE_SIZE)
		TLB_FLUSH("vae1", (v >> PAGE_SHIFT) | TTBR_MAKE_ASID(pmap->asid));
	cache_flush();
}

static void pmap_remove_callback(
	vm_offset_t	v,
	pt_entry_t	*entry,
	void		*data)
{
	pmap_t		pmap;
	phys_addr_t	pa;
	boolean_t	was_present;

	was_present = (entry != PT_ENTRY_NULL) && ((*entry) & AARCH64_PTE_VALID);
	if (!was_present)
		return;

	pa = (phys_addr_t) ((*entry) & AARCH64_PTE_ADDR_MASK);
	*entry = 0;
	pmap = (pmap_t) data;
	pmap->stats.resident_count--;

	pv_unlink(pmap, pa, v);
}

void pmap_remove(
	pmap_t		pmap,
	vm_offset_t	sva,
	vm_offset_t	eva)
{
	int		spl;
	pt_entry_t	*table;
	vm_offset_t	v;

	PMAP_READ_LOCK(pmap, spl);

	table = pmap_table(pmap, sva);
	assert(pmap_table(pmap, eva - 1) == table);

	pmap_walk_range(table, sva, eva, pmap_root_sb(sva), pmap_remove_callback, pmap);

	PMAP_READ_UNLOCK(pmap, spl);

	for (v = sva; v < eva; v += PAGE_SIZE)
		TLB_FLUSH("vae1", (v >> PAGE_SHIFT) | TTBR_MAKE_ASID(pmap->asid));
	cache_flush();
}

void pmap_page_protect(
	phys_addr_t	phys,
	vm_prot_t	prot)
{
	kern_return_t	kr;
	int		spl;
	unsigned long	pai;
	pv_entry_t	pv_h, pv_e, pv_next;
	pmap_t		pmap;
	pt_entry_t	*entry;
	vm_offset_t	v;

	assert(phys != vm_page_fictitious_addr);

	if (!valid_page(phys))
		return;

	PMAP_WRITE_LOCK(spl);

	pai = pa_index(phys);
	// LOCK_PVH(pai);
	pv_h = pai_to_pvh(pai);
	if (pv_h->pmap == PMAP_NULL)
		goto Out;

	pv_e = pv_h;

	do {
		pv_next = pv_e->next;
		pmap = pv_e->pmap;
		simple_lock(&pmap->lock);
		v = pv_e->va;
		kr = pmap_walk(pmap, pmap_table(pmap, v), v, pmap_root_sb(v), &spl, FALSE, VM_PROT_NONE, &entry);
		assert(kr == KERN_SUCCESS);
		assert((*entry) & AARCH64_PTE_VALID);
		assert(((*entry) & AARCH64_PTE_ADDR_MASK) == phys);

		if (prot == VM_PROT_NONE) {
			/*
			 *	We're removing the physical page from all pmaps.
			 *	Don't bother with updating previous pv_e.
			 */
			pmap->stats.resident_count--;
			*entry = 0;
			if (pv_e == pv_h) {
				pv_e->pmap = PMAP_NULL;
				pv_e->va = 0;
				pv_e->next = PV_ENTRY_NULL;
			} else {
				pv_free(pv_e);
			}
		} else {
			/*
			 *	Just reduce the protection.
			 */
			pmap_reduce_prot(entry, pmap_prot(v, prot));
		}

		TLB_FLUSH("vae1", (v >> PAGE_SHIFT) | TTBR_MAKE_ASID(pmap->asid));

		pv_e = pv_next;
	} while (pv_e != PV_ENTRY_NULL);

Out:
	// UNLOCK_PVH(pai);
	PMAP_WRITE_UNLOCK(spl);

	cache_flush();
}

static void pmap_init_mapwindows(void)
{
	kern_return_t		kr;
	pmap_mapwindow_t	*mw;
	vm_offset_t		v;
	int			i, cpu, spl = SPL7;

	v = kernel_virtual_end - MAPWINDOW_SIZE;
	for (cpu = 0; cpu < NCPUS; cpu++) {
		for (i = 0; i < PMAP_NMAPWINDOWS; i++) {
			mw = &percpu_array[cpu].mapwindows[i];
			mw->vaddr = v;
			kr = pmap_walk(kernel_pmap, ttbr1_l0_base, v, VM_AARCH64_T1SZ, &spl, TRUE, VM_PROT_NONE, &mw->entry);
			assert(kr == KERN_SUCCESS);
			*mw->entry &= ~AARCH64_PTE_VALID;
			v += PAGE_SIZE;
		}
	}
}

pmap_mapwindow_t *pmap_get_mapwindow(pt_entry_t entry)
{
	int			i;
	boolean_t		found = FALSE;
	pmap_mapwindow_t	*mw;

	assert(!(entry & ~(AARCH64_PTE_ADDR_MASK | AARCH64_PTE_PROT_MASK)));

	/* Find an empty one.  */
	for (i = 0; i < PMAP_NMAPWINDOWS; i++) {
		mw = &percpu_get(pmap_mapwindow_t, mapwindows[i]);
		if (!(*mw->entry)) {
			found = TRUE;
			break;
		}
	}
	assert(found);
	*mw->entry |= entry | AARCH64_PTE_VALID | AARCH64_PTE_PXN | AARCH64_PTE_UXN;
	cache_flush();
	return mw;
}

void pmap_put_mapwindow(pmap_mapwindow_t *mw)
{
	*mw->entry &= ~AARCH64_PTE_ADDR_MASK & ~AARCH64_PTE_PROT_MASK & ~AARCH64_PTE_VALID;
	// cache_flush();
}

void pmap_zero_page(phys_addr_t p)
{
	vm_offset_t		v;
	pmap_mapwindow_t	*mw;
	boolean_t		direct;

	assert(p != vm_page_fictitious_addr);
	direct = p < VM_PAGE_DIRECTMAP_LIMIT;

	if (direct) {
		v = phystokv(p);
	} else {
		mw = pmap_get_mapwindow((pt_entry_t) p);
		v = mw->vaddr;
	}

	memset((void *) v, 0, PAGE_SIZE);

	if (!direct)
		pmap_put_mapwindow(mw);
}

void pmap_copy_page(phys_addr_t src, phys_addr_t dst)
{
	vm_offset_t		src_v, dst_v;
	pmap_mapwindow_t	*src_mw = NULL, *dst_mw = NULL;
	boolean_t		src_direct, dst_direct;

	assert(src != vm_page_fictitious_addr);
	src_direct = src < VM_PAGE_DIRECTMAP_LIMIT;
	dst_direct = dst < VM_PAGE_DIRECTMAP_LIMIT;

	if (src_direct) {
		src_v = phystokv(src);
	} else {
		src_mw = pmap_get_mapwindow((pt_entry_t) src | AARCH64_PTE_READ_ONLY);
		src_v = src_mw->vaddr;
	}
	if (dst_direct) {
		dst_v = phystokv(dst);
	} else {
		dst_mw = pmap_get_mapwindow((pt_entry_t) dst);
		dst_v = dst_mw->vaddr;
	}

	memcpy((void *) dst_v, (const void *) src_v, PAGE_SIZE);

	if (!src_direct)
		pmap_put_mapwindow(src_mw);
	if (!dst_direct)
		pmap_put_mapwindow(dst_mw);
}
