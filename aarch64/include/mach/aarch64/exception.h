/*
 * Copyright (c) 2023-2024 Free Software Foundation.
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

/*
 *	Codes and subcodes for AArch64 exceptions.
 */
#ifndef	_MACH_AARCH64_EXCEPTION_H_
#define _MACH_AARCH64_EXCEPTION_H_


/*
 *	EXC_BAD_INSTRUCTION
 */

#define EXC_AARCH64_UNK			1	/* unknown reason (unallocated instruction) */
#define EXC_AARCH64_ILL			2	/* illegal execution state */
#define EXC_AARCH64_SVC			3	/* SVC that's not a valid syscall */

/*
 *	EXC_ARITHMETIC
 */

#define EXC_AARCH64_IDF			1	/* input denormal, IDF bit */
#define EXC_AARCH64_IXF			2	/* inexact, IXF bit */
#define EXC_AARCH64_UFF			3	/* underflow, UFF bit */
#define EXC_AARCH64_OFF			4	/* overflow, OFF bit */
#define EXC_AARCH64_DZF			5	/* divide by zero, DZF bit */
#define EXC_AARCH64_IOF			6	/* invalid operation, IOF bit */

/*
 *	EXC_SOFTWARE
 */

/*
 *	EXC_BAD_ACCESS
 *
 *	Exception code normally holds a kern_return_t value (such as
 *	KERN_INVALID_ADDRESS), but for AArch64-specific exceptions these
 *	values can be used as exception code instead; they must not conflict
 *	with kern_return_t values.
 */
#define EXC_AARCH64_AL			100	/* alignment fault */
#define EXC_AARCH64_AL_PC		101	/* misaligned pc */
#define EXC_AARCH64_AL_SP		102	/* misaligned sp */
#define EXC_AARCH64_PAC			103	/* PAC failure */
#define EXC_AARCH64_MTE			104	/* MTE failure */
#define EXC_AARCH64_BTI			105	/* BTI failure, subcode contains BTYPE */

/*
 *	EXC_BREAKPOINT
 */

#define EXC_AARCH64_BRK			1	/* BKPT, BRK */

#endif	/* _MACH_AARCH64_EXCEPTION_H_ */
