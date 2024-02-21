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

#ifndef _AARCH64_TRAP_H_
#define _AARCH64_TRAP_H_

#include <kern/kern_types.h>
#include "aarch64/thread.h"

unsigned int interrupted_pc(thread_t);

void __attribute__((noreturn)) trap_aarch32(void);
void __attribute__((noreturn)) trap_sync_exc_el0(void);
void __attribute__((noreturn)) trap_irq_el0(void);
void __attribute__((noreturn)) trap_fiq_el0(void);
void __attribute__((noreturn)) trap_serror_el0(void);
void trap_sync_exc_el1(unsigned long esr, vm_offset_t far, struct aarch64_kernel_exception_state *akes);
void trap_irq_el1(void);
void trap_fiq_el1(void);
void trap_serror_el1(void);

#endif /* _AARCH64_TRAP_H_ */
