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

#ifndef	_MACH_AARCH64_THREAD_STATUS_H_
#define _MACH_AARCH64_THREAD_STATUS_H_

#define AARCH64_THREAD_STATE	1
#define AARCH64_FLOAT_STATE	2

struct aarch64_thread_state {
	uint64_t x[31];
	uint64_t sp;
	uint64_t pc;
	uint64_t tpidr_el0;
	uint32_t cpsr;
};
#define AARCH64_THREAD_STATE_COUNT	(sizeof (struct aarch64_thread_state) / sizeof(unsigned int))

struct aarch64_float_state {
	__int128 v[32];
	uint32_t fpsr;
	uint32_t fpcr;
};
#define AARCH64_FLOAT_STATE_COUNT	(sizeof (struct aarch64_float_state) / sizeof(unsigned int))

#define AARCH64_REGS_SEGS_STATE 1

#define i386_REGS_SEGS_STATE 1
#define i386_THREAD_STATE_COUNT 1
struct i386_thread_state {int a;};

#endif	/* _MACH_AARHC64_THREAD_STATUS_H_ */
