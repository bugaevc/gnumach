/*
 *  Copyright (C) 2024 Free Software Foundation
 *
 * This program is free software ; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation ; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY ; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the program ; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <mach/machine/asm.h>

	.section .text.boot
.boot_header:
	/* See https://docs.kernel.org/arch/arm64/booting.html#call-the-kernel-image */
	b	EXT(_start)
	nop
	.dword	0				/* ??? */
	.dword	EXT(_image_size)
	.dword	1
	.dword	0
	.dword	0
	.dword	0
	.dword	0x644d5241
	.dword	0

ENTRY(_start)
	mrs	x4, CurrentEL
	cmp	x4, #(1 << 2)
	beq	.in_el1
	adr	x5, .in_el1
	mov	x6, #((0x7 << 6) | (5 << 0))
	cmp	x4, #(2 << 2)
	beq	.in_el2
.in_el3:
	msr	ELR_EL3, x5
	msr	SPSR_EL3, x6
	eret
.in_el2:
	msr	ELR_EL2, x5
	msr	SPSR_EL2, x6
	eret
.in_el1:
	adr	x1, .boot_stack_end
	mov	sp, x1

#if 0
        // enable FPU for vector instructions... or should we disable them?
        mrs     x1, cpacr_el1
        mov     x2, #(3 << 20)
        orr     x2, x1, x2
        msr     cpacr_el1, x2
#endif

	b	EXT(c_boot_entry)
END(_start)

	.bss
.boot_stack:
	.space	4096
.boot_stack_end:
