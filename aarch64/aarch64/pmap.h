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

typedef struct pmap *pmap_t;

#define pmap_attribute(pmap,addr,size,attr,value) (KERN_INVALID_ADDRESS)
#define PMAP_NULL ((pmap_t) 0)

typedef struct {
	pt_entry_t	*entry;
	vm_offset_t	vaddr;
} pmap_mapwindow_t;

extern pmap_mapwindow_t *pmap_get_mapwindow(pt_entry_t entry);
extern void pmap_put_mapwindow(pmap_mapwindow_t *map);

#define PMAP_NMAPWINDOWS 2	/* per CPU */

extern vm_offset_t kernel_virtual_start;
extern vm_offset_t kernel_virtual_end;

extern void pmap_activate_user(pmap_t pmap);

#define PMAP_ACTIVATE_KERNEL(my_cpu)
#define PMAP_DEACTIVATE_KERNEL(my_cpu)
#define PMAP_DEACTIVATE_USER(pmap, th, my_cpu)

#define PMAP_ACTIVATE_USER(pmap, th, my_cpu) 				\
MACRO_BEGIN								\
	(void) (th);							\
	(void) (my_cpu);						\
	if (likely((pmap) != kernel_pmap))				\
		pmap_activate_user(pmap);				\
MACRO_END

#define pmap_kernel()                   (kernel_pmap)
#define pmap_phys_address(frame)	(frame)
#define pmap_phys_to_frame(phys)        (phys)
#define pmap_copy(dst_pmap,src_pmap,dst_addr,len,src_addr)

extern integer_t	pmap_resident_count(pmap_t pmap);

static inline boolean_t	pmap_is_modified(phys_addr_t) { return FALSE; }
static inline boolean_t	pmap_is_referenced(phys_addr_t) { return FALSE; }
static inline void	pmap_clear_modify(phys_addr_t) {}
static inline void	pmap_clear_reference(phys_addr_t) {}


struct dtb_node;
extern void pmap_discover_physical_memory(const struct dtb_node *node);
extern void pmap_bootstrap(void);
extern void pmap_bootstrap_misc(void);

extern void pmap_zero_page(phys_addr_t);
extern void pmap_copy_page(phys_addr_t, phys_addr_t);

#endif /* _AARCH64_PMAP_ */
