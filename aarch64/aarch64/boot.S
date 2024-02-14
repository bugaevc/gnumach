/*
 *  Copyright (C) 2022 Free Software Foundation
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

	.section .text.boot
	.globl  _start
	.type _start, @function
_start:
	mrs	x1, CurrentEL
	cmp	x1, #(1 << 2)
	beq	.in_el1
	cmp	x1, #(2 << 2)
	beq	.in_el2
.in_el3:
	adr	x1, .in_el1
	msr	ELR_EL3, x1
	ldr	x1, =((0x7 << 6) | (5 << 0))
	msr	SPSR_EL3, x1
	eret
.in_el2:
	adr	x1, .in_el1
	msr	ELR_EL2, x1
	ldr	x1, =((0x7 << 6) | (5 << 0))
	msr	SPSR_EL2, x1
	eret
.in_el1:
	adr	x1, .boot_stack_end
	mov	sp, x1

        // enable FPU for vector instructions... or should we disable them?
        mrs     x1, cpacr_el1
        mov     x0, #(3 << 20)
        orr     x0, x1, x0
        msr     cpacr_el1, x0

	adr	x0, _bstart
	adr	x2, _bend
	sub	x2, x2, x0
	mov	x1, xzr
	bl	memset

	b	c_boot_entry
	.size _start, . - _start

	.bss
.boot_stack:
	.space	4096
.boot_stack_end:
