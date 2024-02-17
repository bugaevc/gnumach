#include "aarch64/pmap.h"
#include "aarch64/vm_param.h"
#include <vm/pmap.h>
#include <vm/vm_page.h>
#include <kern/slab.h>
#include <kern/printf.h>
#include <string.h>

static boolean_t	pmap_debug = FALSE;
static boolean_t	pmap_initialized = FALSE;

static struct pmap	kernel_pmap_store;
pmap_t			kernel_pmap;
static pt_entry_t	*ttbr1_l0_base;

static struct kmem_cache pmap_cache;
static struct kmem_cache table_cache;

/*
 *	Range of kernel virtual addresses available for kernel memory mapping.
 *	Does not include the virtual addresses used to map physical memory 1-1.
 *	Initialized by pmap_bootstrap.
 */
vm_offset_t	kernel_virtual_start;
vm_offset_t	kernel_virtual_end;

/*
 * Two slots for temporary physical page mapping, to allow for
 * physical-to-physical transfers.
 */
// static pmap_mapwindow_t mapwindows[PMAP_NMAPWINDOWS * NCPUS];
#define MAPWINDOW_SIZE (PMAP_NMAPWINDOWS * NCPUS * PAGE_SIZE)

static inline void cache_flush(void)
{
	asm volatile("dsb st\n\tisb sy" ::: "memory");
}

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	Allocate the kernel page translation tables,
 *	and direct-map all physical memory.
 *	Called with mapping off.
 */
void pmap_bootstrap(void)
{
	uintptr_t	scratch1, scratch2;
	pt_entry_t	*phys_ttbr1_l0_base;
	pt_entry_t	*phys_ttbr0_l0_base;
	pt_entry_t	*ttbr0_l0_base;

	phys_ttbr0_l0_base = (pt_entry_t*)pmap_grab_page();
	ttbr0_l0_base = (pt_entry_t*)phystokv(phys_ttbr0_l0_base);

	phys_ttbr1_l0_base = (pt_entry_t*)pmap_grab_page();
	ttbr1_l0_base = (pt_entry_t*)phystokv(phys_ttbr1_l0_base);

	memset(phys_ttbr0_l0_base, 0, PAGE_SIZE);
	/* Temporary identity map.  */
	phys_ttbr0_l0_base[1] = 0x40000000 | AARCH64_PTE_MAIR_INDEX(MAIR_NORMAL_INDEX) | AARCH64_PTE_ACCESS | AARCH64_PTE_BLOCK | AARCH64_PTE_VALID | AARCH64_PTE_UXN | AARCH64_PTE_UNO_PRW | AARCH64_PTE_NON_SH /* ? */;

	/* This would need to be load slide rather than 0 for PIC.  */
	memset(phys_ttbr1_l0_base, 0, PAGE_SIZE);
	phys_ttbr1_l0_base[0] = 0x0 | AARCH64_PTE_MAIR_INDEX(MAIR_NORMAL_INDEX) | AARCH64_PTE_ACCESS | AARCH64_PTE_BLOCK | AARCH64_PTE_VALID | AARCH64_PTE_UXN | AARCH64_PTE_UNO_PRW | AARCH64_PTE_NON_SH /* ? */;
	phys_ttbr1_l0_base[1] = 0x40000000 | AARCH64_PTE_MAIR_INDEX(MAIR_NORMAL_INDEX) | AARCH64_PTE_ACCESS | AARCH64_PTE_BLOCK | AARCH64_PTE_VALID | AARCH64_PTE_UXN | AARCH64_PTE_UNO_PRW | AARCH64_PTE_NON_SH /* ? */;

	/* Attempt to enable the MMU.  */
	asm volatile(
		/* Load special register values.  */
		"msr mair_el1, %[mair]\n\t"
		"msr ttbr0_el1, %[ttbr0]\n\t"
		"msr ttbr1_el1, %[ttbr1]\n\t"
		"msr tcr_el1, %[tcr]\n\t"
		"isb sy\n\t"
		/* Enable the MMU bit in sctlr_el1.  */
		"mrs %[scratch1], sctlr_el1\n\t"
		"orr %[scratch1], %[scratch1], #1\n\t"
		"msr sctlr_el1, %[scratch1]\n\t"
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
		/* Now adjust the saved x29 / x30.  */
		"ldp %[scratch2], x30, [x29]\n\t"
		"add %[scratch2], %[scratch2], %[scratch1]\n\t"
		"add x30, x30, %[scratch1]\n\t"
		"stp %[scratch2], x30, [x29]"
		:
		[scratch1] "=&r"(scratch1),
		[scratch2] "=&r"(scratch2)
		:
		[mair]	"r"(MAIR_VALUE),
		[ttbr0]	"r"(phys_ttbr0_l0_base),
		[ttbr1]	"r"(phys_ttbr1_l0_base),
		[tcr]	"r"(TCR_VALUE),
		[min_addr] "i"(VM_MIN_KERNEL_ADDRESS)
		: "memory"
	);

	/* Unmap the identity mapping, we no longer need it.  */
	ttbr0_l0_base[1] = 0;
	cache_flush();
	/* TODO free the page */
}

void pmap_bootstrap_misc(void)
{
	/*
	 * Determine the kernel virtual address range.
	 * It starts at the end of the physical memory
	 * mapped into the kernel address space,
	 * and extends to a stupid arbitrary limit beyond that.
	 */
	kernel_virtual_start = phystokv(/* FIXME */ 0x80000000);
	kernel_virtual_end = kernel_virtual_start + VM_KERNEL_MAP_SIZE;


	kernel_pmap = &kernel_pmap_store;
#if NCPUS > 1
	lock_init(&pmap_system_lock, FALSE);
#endif
	simple_lock_init(&kernel_pmap->lock);
	kernel_pmap->ref_count = 1;
	kernel_pmap->l0_base = PT_ENTRY_NULL;
}

void pmap_virtual_space(
	vm_offset_t *startp,
	vm_offset_t *endp)
{
	*startp = kernel_virtual_start;
	*endp = kernel_virtual_end - MAPWINDOW_SIZE;
}

void load_ttbr0(pmap_t p)
{
	asm volatile("msr ttbr0_el1, %0" :: "r"(kvtophys(p->l0_base)));
	cache_flush();
}

void pmap_init(void)
{
	kmem_cache_init(&pmap_cache, "pmap", sizeof(struct pmap), alignof(struct pmap), NULL, 0);
	kmem_cache_init(&table_cache, "page table", PAGE_SIZE, PAGE_SIZE, NULL, KMEM_CACHE_PHYSMEM);

	pmap_initialized = TRUE;
}

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

	p->ref_count = 1;
	simple_lock_init(&p->lock);

	return p;
}

void pmap_reference(pmap_t pmap)
{
	simple_lock(&pmap->lock);
	pmap->ref_count++;
	simple_unlock(&pmap->lock);
}

void pmap_destroy(pmap_t pmap)
{
	int c;

	simple_lock(&pmap->lock);
	c = --pmap->ref_count;
	simple_unlock(&pmap->lock);

	if (c != 0)
		return;

	/* TODO */
	printf("pmap_destroy()\n");
}

static pt_entry_t pmap_prot(vm_offset_t v, vm_prot_t prot)
{
	pt_entry_t	entry = 0;

	if (v >= VM_MIN_KERNEL_ADDRESS) {
		if (prot & VM_PROT_WRITE)
			entry |= AARCH64_PTE_UNO_PRW;
		else
			entry |= AARCH64_PTE_UNO_PRO;
		assert(!(prot & VM_PROT_EXECUTE));
		entry |= AARCH64_PTE_PXN;
		entry |= AARCH64_PTE_UXN;
	} else {
		if (prot & VM_PROT_WRITE)
			entry |= AARCH64_PTE_URW_PRW;
		else
			entry |= AARCH64_PTE_URO_PRO;
		entry |= AARCH64_PTE_PXN;
		if (!(prot & VM_PROT_EXECUTE))
			entry |= AARCH64_PTE_UXN;
	}

	return entry;
}

#define BITS_PER_LEVEL	9	/* 4K granularity */

static kern_return_t pmap_walk(
	pmap_t		pmap,
	pt_entry_t	*table,
	vm_offset_t	v,
	int		significant_bits,
	boolean_t	create,
	vm_prot_t	prot,
	pt_entry_t	**out_entry)
{
	unsigned long	index;
	pt_entry_t	entry;
	int		next_sb;
	boolean_t	last_level;
	pt_entry_t	*next_table;

	if (pmap_debug) printf("pmap_walk(table = %p, v = %#016.x, sb = %d)\n", table, (unsigned long long) v, significant_bits);

	assert(significant_bits > PAGE_SHIFT);
	next_sb = (significant_bits - PAGE_SHIFT - 1) / BITS_PER_LEVEL * BITS_PER_LEVEL + PAGE_SHIFT;
	last_level = (next_sb == PAGE_SHIFT);

	index = (v >> next_sb) & ((1 << (significant_bits - next_sb)) - 1);
	if (pmap_debug) printf("index = %d\n", index);
	assert(index < PAGE_SIZE / sizeof(pt_entry_t));
	entry = table[index];
	if (!(entry & AARCH64_PTE_VALID)) {
		if (!create)
			return KERN_INVALID_ADDRESS;

		if (!last_level) {
			/*
			 *	This PTE will point to the next-level table.
			 *	Allocate and clear that now.
			 */
			if (!pmap_initialized) {
				next_table = (pt_entry_t*)phystokv(vm_page_bootalloc(PAGE_SIZE));
			} else {
				while (!(next_table = (pt_entry_t*)kmem_cache_alloc(&table_cache)))
					VM_PAGE_WAIT(0);
			}
			memset(next_table, 0, PAGE_SIZE);
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
	return pmap_walk(pmap, next_table, v, next_sb, create, prot, out_entry);
}

static pt_entry_t *pmap_table(pmap_t pmap, vm_offset_t v)
{
	if (v >= VM_MIN_KERNEL_ADDRESS)
		return ttbr1_l0_base;
	assert(pmap != kernel_pmap);
	return pmap->l0_base;
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

	assert(pmap != NULL);
	assert(pa != vm_page_fictitious_addr);
	if (pmap_debug) printf("pmap_enter(%#016.x, %#llx)\n", v, (unsigned long long) pa);

	if (pmap == kernel_pmap && (v < kernel_virtual_start || v >= kernel_virtual_end))
		panic("pmap_enter(%#016.x, %#llx) falls in physical memory area!\n", (unsigned long) v, (unsigned long long) pa);

	if (pmap != kernel_pmap && v >= kernel_virtual_start)
		panic("pmap_enter(%#016.x, %#llx) for a non-kernel pmap?\n", (unsigned long) v, (unsigned long long) pa);

	kr = pmap_walk(pmap, pmap_table(pmap, v), v, 36, TRUE, prot, &entry);
	assert(kr == KERN_SUCCESS);
	*entry = ((*entry) & ~AARCH64_PTE_ADDR_MASK) | (pt_entry_t)pa;
	cache_flush();
}


phys_addr_t pmap_extract(
	pmap_t		pmap,
	vm_offset_t	v)
{
	kern_return_t	kr;
	pt_entry_t	*entry;

	assert(pmap != NULL);

	kr = pmap_walk(pmap, pmap_table(pmap, v), v, 36, FALSE, VM_PROT_NONE, &entry);
	if (kr != KERN_SUCCESS)
		return 0;
	return (*entry) & AARCH64_PTE_ADDR_MASK;
}

void pmap_protect(
	pmap_t		pmap,
	vm_offset_t	sva,
	vm_offset_t	eva,
	vm_prot_t	prot)
{
	kern_return_t	kr;
	vm_offset_t	v;
	pt_entry_t	*table;
	pt_entry_t	*entry;

	assert(pmap != NULL);
	assert(sva < eva);

	table = pmap_table(pmap, sva);
	assert(pmap_table(pmap, eva - 1) == table);

	/* TODO: don't pmap_walk() each iteration */
	for (v = sva; v != eva; v += PAGE_SIZE) {
		kr = pmap_walk(pmap, table, v, 36, FALSE, VM_PROT_NONE, &entry);
		assert(kr == KERN_SUCCESS);
		*entry = ((*entry) & ~AARCH64_PTE_PROT_MASK) | pmap_prot(v, prot);
	}
	cache_flush();
}
