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

#ifndef _AARCH64_PMAP_
#define _AARCH64_PMAP_

#include <kern/lock.h>
#include <mach/machine/vm_param.h>
#include <mach/vm_statistics.h>
#include <mach/kern_return.h>

typedef phys_addr_t pt_entry_t;
#define PT_ENTRY_NULL	((pt_entry_t *) 0)

struct pmap {
	pt_entry_t	*l0_base;	/* TTBR0 */
	int		ref_count;
	decl_simple_lock_data(,lock)	/* lock on map */
	struct pmap_statistics stats;	/* map statistics */
};

typedef struct pmap *pmap_t;

#define pmap_attribute(pmap,addr,size,attr,value) (KERN_INVALID_ADDRESS)
#define PMAP_NULL ((pmap_t) 0)

#define PMAP_NMAPWINDOWS 2	/* Per CPU */

extern vm_offset_t kernel_virtual_start;
extern vm_offset_t kernel_virtual_end;

/* PTE bits */
#define AARCH64_PTE_ADDR_MASK	0x000ffffffffff000UL
#define AARCH64_PTE_PROT_MASK	0x00600000000000c0UL

/* Block or table */
#define AARCH64_PTE_BLOCK	0x0000000000000000UL	/* points to a block of phys memory */
#define AARCH64_PTE_TABLE	0x0000000000000002UL	/* points to a next level table */
#define AARCH64_PTE_LEVEL3	0x0000000000000002UL	/* this is a level 3 PTE (same value as table) */

#define AARCH64_PTE_VALID	0x0000000000000001UL	/* this entry is valid */
#define AARCH64_PTE_NS		0x0000000000000020UL	/* security bit (only EL3 & secure EL1) */
#define AARCH64_PTE_ACCESS	0x0000000000000400UL	/* if unset, trap on access */
#define AARCH64_PTE_BTI		0x0004000000000000UL	/* enable branch target identification */
#define AARCH64_PTE_PXN		0x0020000000000000UL	/* privileged execute never */
#define AARCH64_PTE_UXN		0x0040000000000000UL	/* unprivileged execute never */

#define AARCH64_PTE_MAIR_INDEX(i) ((i) << 2)		/* cache policies, as an index into MAIR table */

/* Access permissions */
#define AARCH64_PTE_UNO_PRW	0x0000000000000000UL	/* no unprivileged access, R/W privileged */
#define AARCH64_PTE_URW_PRW	0x0000000000000040UL	/* R/W both privileged and unprivileged */
#define AARCH64_PTE_UNO_PRO	0x0000000000000080UL	/* no unprivileged access, R/O privileged */
#define AARCH64_PTE_URO_PRO	0x00000000000000c0UL	/* R/O both privileged and unprivileged */

/* Shareability */
#define AARCH64_PTE_NON_SH	0x0000000000000000UL	/* non-shareable */
#define AARCH64_PTE_OUTER_SH	0x0000000000000200UL	/* outer shareable */
#define AARCH64_PTE_INNER_SH	0x0000000000000300UL	/* inner shareable */


#define MAIR_NORMAL_INDEX	0
#define MAIR_NORMAL_FLAGS	0xff
#define MAIR_DEVICE_INDEX	1
#define MAIR_DEVICE_FLAGS	0x00

#define MAIR_VALUE_ENTRY(index, flags)		((flags) << ((index) * 8))
#define MAIR_VALUE		(MAIR_VALUE_ENTRY(MAIR_NORMAL_INDEX, MAIR_NORMAL_FLAGS) | MAIR_VALUE_ENTRY(MAIR_DEVICE_INDEX, MAIR_DEVICE_FLAGS))

#define TCR_T0SZ(size)		(64 - size)
#define TCR_TG0_4K		0x0000000000000000UL
#define TCR_TG0_64K		0x0000000000004000UL
#define TCR_TG0_16K		0x0000000000008000UL

#define TCR_T1SZ(size)		((64 - size) << 16)
#define TCR_TG1_16K		0x0000000040000000UL
#define TCR_TG1_4K		0x0000000080000000UL
#define TCR_TG1_64K		0x00000000c0000000UL

#define TCR_VALUE		(TCR_T0SZ(36) | TCR_TG0_4K | TCR_T1SZ(36) | TCR_TG1_4K)

#define SCTLR_M			0x0000000000000001UL	/* enable MMU */
#define SCTLR_A			0x0000000000000002UL	/* enable alignment checking */
#define SCTLR_SA		0x0000000000000008UL	/* enable SP alignment checking in EL1 */
#define SCTLR_SA0		0x0000000000000010UL	/* enable SP alignment checking in EL0 */
#define SCTLR_ENDB		0x0000000000002000UL	/* PAC */
#define SCTLR_SPAN		0x0000000000800000UL	/* set psate.PAN upon an exception to EL1 */
#define SCTLR_ENDA		0x0000000008000000UL	/* PAC */
#define SCTLR_ENIB		0x0000000040000000UL	/* PAC */
#define SCTLR_ENIA		0x0000000080000000UL	/* PAC */
#define SCTLR_BT0		0x0000000800000000UL	/* enable BTI-on-PACI?SP traps in EL0 */
#define SCTLR_BT1		0x0000001000000000UL	/* enable BTI-on-PACI?SP traps in EL1 */

#define SCTLR_VALUE		(SCTLR_M | SCTLR_SA | SCTLR_SA0 /*| SCTLR_SPAN*/ | SCTLR_BT1)

extern void load_ttbr0(pmap_t p);

#define PMAP_ACTIVATE_KERNEL(my_cpu)
#define PMAP_DEACTIVATE_KERNEL(my_cpu)
#define PMAP_ACTIVATE_USER(pmap, th, my_cpu) ((void)th,(void)my_cpu,load_ttbr0(pmap))
#define PMAP_DEACTIVATE_USER(pmap, th, my_cpu)

#define pmap_resident_count(pmap) 123456

#define pmap_kernel()                   (kernel_pmap)
#define pmap_phys_address(frame)	(frame)
#define pmap_phys_to_frame(phys)        1234
#define pmap_copy(dst_pmap,src_pmap,dst_addr,len,src_addr)

extern void pmap_bootstrap(void);
extern void pmap_bootstrap_misc(void);

/*
 *  pmap_zero_page zeros the specified (machine independent) page.
 */
extern void pmap_zero_page(phys_addr_t);

/*
 *  pmap_copy_page copies the specified (machine independent) pages.
 */
extern void pmap_copy_page(phys_addr_t, phys_addr_t);

#endif /* _AARCH64_PMAP_ */
