#include "aarch64/pmap.h"
#include "aarch64/vm_param.h"
#include <vm/pmap.h>

static struct pmap	kernel_pmap_store;
pmap_t			kernel_pmap;
static pt_entry_t	*ttbr1_l0_base;

/*
 *	Range of kernel virtual addresses available for kernel memory mapping.
 *	Does not include the virtual addresses used to map physical memory 1-1.
 *	Initialized by pmap_bootstrap.
 */
vm_offset_t kernel_virtual_start;
vm_offset_t kernel_virtual_end;

/*
 * Two slots for temporary physical page mapping, to allow for
 * physical-to-physical transfers.
 */
// static pmap_mapwindow_t mapwindows[PMAP_NMAPWINDOWS * NCPUS];
#define MAPWINDOW_SIZE (PMAP_NMAPWINDOWS * NCPUS * PAGE_SIZE)

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	Allocate the kernel page translation tables,
 *	and direct-map all physical memory.
 *	Called with mapping off.
 */
void pmap_bootstrap(void)
{
	uint64_t sctrl;
	pt_entry_t *kernel_l1_base, *kernel_l2_base, *kernel_l3_base;

	kernel_pmap = &kernel_pmap_store;
#if NCPUS > 1
	lock_init(&pmap_system_lock, FALSE);
#endif
	simple_lock_init(&kernel_pmap->lock);
	kernel_pmap->ref_count = 1;

	/*
	 * Determine the kernel virtual address range.
	 * It starts at the end of the physical memory
	 * mapped into the kernel address space,
	 * and extends to a stupid arbitrary limit beyond that.
	 */
	kernel_virtual_start = phystokv(/* FIXME */ 0x80000000);
	kernel_virtual_end = kernel_virtual_start + VM_KERNEL_MAP_SIZE;

	ttbr1_l0_base = (pt_entry_t*)phystokv(pmap_grab_page());
#if 0
	kernel_l1_base = (pt_entry_t*)phystokv(pmap_grab_page());
	kernel_l2_base = (pt_entry_t*)phystokv(pmap_grab_page());
	kernel_l3_base = (pt_entry_t*)phystokv(pmap_grab_page());

	ttbr1_l0_base[0] = kvtophys(kernel_l1_base) | PT_PAGE | PT_AF | PT_KERNEL | PT_ISH | PT_MEM;
	ttbr1_l1_base[0] = kvtophys(kernel_l2_base) | PT_PAGE | PT_AF | PT_KERNEL | PT_ISH | PT_MEM;
	ttbr1_l2_base[0] = kvtophys(kernel_l3_base) | PT_PAGE | PT_AF | PT_KERNEL | PT_ISH | PT_MEM;
	for (int i = 0; i < 512; i++)
	{
		ttbr1_l3_base[i] = kvtophys() | PT_PAGE | PT_AF | PT_KERNEL | PT_ISH;
	}

	/* Attempt to enable the MMU.  */
	asm volatile("msr mair_el1, %0" :: "r"(0x4404ff));
	asm volatile("msr tcr_el1, %0" :: "r"(0)); /* FIXME */
	asm volatile("msr ttbr1_el1, %0", :: "r"(ttbr1_l0_base));
	asm("dsb ish; mrs %0, sctrl_el1" : "=r"(sctrl));
	sctrl |= 0xc00801;
	sctrl &= ~0x308101e;
	asm volatile("msr sctrl_el1, %0; isb" :: "r"(sctrl));
#endif
}

void pmap_virtual_space(
	vm_offset_t *startp,
	vm_offset_t *endp)
{
	*startp = kernel_virtual_start;
	*endp = kernel_virtual_end - MAPWINDOW_SIZE;
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
	pmap_t			pmap,
	vm_offset_t		v,
	phys_addr_t		pa,
	vm_prot_t		prot,
	boolean_t		wired)
{
}
