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

#ifndef _AARCH64_LOCORE_
#define _AARCH64_LOCORE_

#include <mach/mach_types.h>
#include <kern/sched_prim.h>
#include <stddef.h>

/*
 *	Fault recovery in copyin/copyout routines.
 *
 *	Both offsets are relative to &recover_table.
 */
struct recovery {
	vm_offset_t	fault_addr_off;
	vm_offset_t	recover_addr_off;
};
extern const struct recovery recover_table[];
extern const struct recovery recover_table_end[];

int copyin(const void *userbuf, void *kernelbuf, size_t cn);
int copyout(const void *kernelbuf, void *userbuf, size_t cn);

extern void __attribute__((noreturn)) call_continuation(continuation_t continuation);
extern boolean_t handle_syscall(struct aarch64_thread_state *ats);

extern void load_exception_vector_table(void);

struct aarch64_float_state;
extern void _fpu_save_state(struct aarch64_float_state *);
extern void _fpu_load_state(const struct aarch64_float_state *);

#endif /* _AARCH64_LOCORE_ */
