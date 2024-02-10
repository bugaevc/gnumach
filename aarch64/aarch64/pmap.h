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

struct pmap {
	pt_entry_t	*l0_base;	/* TTBR0 */
	int		ref_count;
	decl_simple_lock_data(,lock)	/* lock on map */
	struct pmap_statistics stats;	/* map statistics */
};

typedef struct pmap *pmap_t;

#define pmap_attribute(pmap,addr,size,attr,value)   0  // FIXME
#define PMAP_NULL ((pmap_t) 0)

#define PMAP_NMAPWINDOWS 2	/* Per CPU */

extern vm_offset_t kernel_virtual_start;
extern vm_offset_t kernel_virtual_end;

#define PMAP_ACTIVATE_KERNEL(my_cpu)
#define PMAP_DEACTIVATE_KERNEL(my_cpu)
#define PMAP_ACTIVATE_USER(pmap, th, my_cpu)
#define PMAP_DEACTIVATE_USER(pmap, th, my_cpu)

#define pmap_resident_count(pmap) 123456

#define pmap_kernel()                   (kernel_pmap)
#define pmap_phys_address(frame)        1234
#define pmap_phys_to_frame(phys)        1234
#define pmap_copy(dst_pmap,src_pmap,dst_addr,len,src_addr)

extern void pmap_bootstrap(void);

/*
 *  pmap_zero_page zeros the specified (machine independent) page.
 */
extern void pmap_zero_page (phys_addr_t);

/*
 *  pmap_copy_page copies the specified (machine independent) pages.
 */
extern void pmap_copy_page (phys_addr_t, phys_addr_t);

#endif /* _AARCH64_PMAP_ */
