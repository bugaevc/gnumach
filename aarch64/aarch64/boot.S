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
#include "aarch64/bits/spsr.h"
#include "aarch64/bits/hcr.h"
#include "aarch64/bits/id_aa64.h"

	.section .text.boot
.boot_header:
	/* See https://docs.kernel.org/arch/arm64/booting.html#call-the-kernel-image */
	b	EXT(_start)
	nop
	.dword	0				/* ??? */
	.dword	EXT(_image_end) - .boot_header
	.dword	1
	.dword	0
	.dword	0
	.dword	0
	.dword	0x644d5241
	.dword	0

ENTRY(_start)
	/*
	 *	See what EL we're entered in.
	 *
	 *	If it's EL1, there's nothing special to do, and we can proceed.
	 *
	 *	If it's EL2, see if FEAT_VHE is supported, and if so, enable
	 *	E2H, and proceed booting in EL2.  If it's unsupported, drop
	 *	to AArch64 EL1 and boot there.
	 *
	 *	If it's EL3, drop down to EL2 and do the same things.
	 */
	mrs	x4, CurrentEL
	cmp	x4, #(SPSR_MAKE_EL(1))
	b.eq	2f

	cmp	x4, #(SPSR_MAKE_EL(2))
	b.eq	1f

	/* Must be in EL3, drop down to EL2.  */
	adr	x5, 1f
	msr	ELR_EL3, x5
	mov	x6, #(SPSR_DAIF | SPSR_MAKE_EL(2) | SPSR_SPSEL_N)
	msr	SPSR_EL3, x6
	eret

0:
	/* EL2 without VHE, drop down to EL1.  */
	mov	x7, #(HCR_RW)
	msr	HCR_EL2, x7
	adr	x5, 2f
	msr	ELR_EL2, x5
	mov	x6, #(SPSR_DAIF | SPSR_MAKE_EL(1) | SPSR_SPSEL_N)
	msr	SPSR_EL2, x6
	eret

1:
#ifdef notyet
	/* EL2, see if FEAT_VHE is supported.  */
	mrs	x2, ID_AA64MMFR1_EL1
	tbz	x2, #(ID_AA64MMFR1_VH_SHIFT), 0b
	/* Enable E2H.  */
	mov	x5, #((HCR_E2H | HCR_RW) & 0xffffffff)
	movk	x5, #((HCR_E2H | HCR_RW) >> 32), lsl #32
	msr	HCR_EL2, x5
	isb
#else
	b	0b
#endif

2:
	adr	x1, .boot_stack_end
	mov	sp, x1

	b	EXT(c_boot_entry)
END(_start)

	.bss
.boot_stack:
	.space	4096
.boot_stack_end:

	.section .note.GNU-stack,"",%progbits
