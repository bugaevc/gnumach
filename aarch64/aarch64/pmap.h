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

struct pmap {
  //TODO registers
  decl_simple_lock_data(,lock)	/* lock on map */
  struct pmap_statistics stats;	/* map statistics */
};

typedef struct pmap *pmap_t;

#define pmap_attribute(pmap,addr,size,attr,value)   0  // FIXME
#define PMAP_NULL ((pmap_t) 0)

extern vm_offset_t kernel_virtual_start;
extern vm_offset_t kernel_virtual_end;

#endif /* _AARCH64_PMAP_ */
