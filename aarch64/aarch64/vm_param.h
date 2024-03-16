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

#ifndef _AARCH64_VM_PARAM_
#define _AARCH64_VM_PARAM_

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define KERNEL_STACK_SIZE  (4*PAGE_SIZE)

#define VM_MIN_USER_ADDRESS VM_MIN_ADDRESS
#define VM_MAX_USER_ADDRESS VM_MAX_ADDRESS

#define VM_MIN_KERNEL_ADDRESS (0xffff000000000000ULL)
#define VM_MAX_KERNEL_ADDRESS (0xFFFFFFFFFFFF0000ULL)

#define VM_AARCH64_T0SZ		48
#define VM_AARCH64_T1SZ		48

#define CPU_L1_SHIFT 6

#define VM_PAGE_DMA_LIMIT       DECL_CONST(0x1000000, UL)
#define VM_PAGE_DMA32_LIMIT     DECL_CONST(0x100000000, UL)
#define VM_PAGE_DIRECTMAP_LIMIT DECL_CONST(0x400000000000, UL)
#define VM_PAGE_HIGHMEM_LIMIT   DECL_CONST(0x10000000000000, ULL)

#define VM_PAGE_SEG_DMA         0
#define VM_PAGE_SEG_DMA32       (VM_PAGE_SEG_DMA+1)
#define VM_PAGE_SEG_DIRECTMAP   (VM_PAGE_SEG_DMA32+1)
#define VM_PAGE_SEG_HIGHMEM     (VM_PAGE_SEG_DIRECTMAP+1)

#define VM_PAGE_MAX_SEGS 4

#define phystokv(a)	((vm_offset_t)(a) + VM_MIN_KERNEL_ADDRESS)
#define kvtophys(a)	((vm_offset_t)(a) - VM_MIN_KERNEL_ADDRESS)

#define VM_KERNEL_MAP_SIZE (512 * 1024 * 1024)

#include <mach/vm_param.h>

#endif /* _AARCH64_VM_PARAM_ */
