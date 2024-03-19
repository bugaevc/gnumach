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

#ifndef _AARCH64_PERCPU_
#define _AARCH64_PERCPU_


struct percpu;

#if NCPUS > 1
static inline __attribute__((const)) struct percpu *_my_percpu(void)
{
	struct percpu	*p;

	asm("mrs %0, TPIDR_EL1" : "=r"(p));
	return p;
}
#else
#define _my_percpu()			(&percpu_array[0])
#endif

#define percpu_assign(stm, val)		_my_percpu()->stm = (val)
#define percpu_get(typ, stm)		(_my_percpu()->stm)
#define percpu_ptr(type, stm)		(&_my_percpu()->stm)

#include <kern/processor.h>
#include <kern/kern_types.h>
#include "aarch64/pmap.h"

struct percpu {
	struct processor	processor;
	thread_t		active_thread;
	vm_offset_t		active_stack;
	boolean_t		in_irq_from_el0 : 1;
	thread_t		fpu_thread;
	pmap_mapwindow_t	mapwindows[PMAP_NMAPWINDOWS];
/*
    struct machine_slot	machine_slot;
    ast_t		need_ast;
    ipc_kmsg_t		ipc_kmsg_cache;
    pmap_update_list	cpu_update_list;
    spl_t		saved_ipl;
    timer_data_t	kernel_timer;
    timer_t		current_timer;
    unsigned long	in_interrupt;
*/
};

extern struct percpu percpu_array[NCPUS];

void init_percpu(int cpu);

#endif /* _AARCH64_PERCPU_ */
