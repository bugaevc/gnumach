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

#include "aarch64/fpu.h"
#include "aarch64/locore.h"
#include <kern/slab.h>
#include <string.h>

#define FPEN_TRAP_MASK			0x300000
#define FPEN_TRAP_EL01			0x000000	/* trap FP at EL0 & EL1 */
#define FPEN_TRAP_EL0			0x100000	/* trap FP at EL0 */
#define FPEN_TRAP_NONE			0x300000	/* don't trap FP */

static struct kmem_cache	fpu_cache;

void fpu_init(void)
{
	kmem_cache_init(&fpu_cache, "fpu", sizeof(struct aarch64_float_state), alignof(struct aarch64_float_state), NULL, 0);
}

static void fpen(boolean_t enable)
{
	uint64_t	cpacr;

	asm("mrs %0, CPACR_EL1" : "=r"(cpacr));

	if (enable)
		cpacr = (cpacr & ~FPEN_TRAP_MASK) | FPEN_TRAP_NONE;
	else
		cpacr = (cpacr & ~FPEN_TRAP_MASK) | FPEN_TRAP_EL01;

	asm volatile("msr CPACR_EL1, %0" :: "r"(cpacr));
	asm volatile("isb");
}

void fpu_switch_context(thread_t new)
{
	thread_t	fpu_thread;

	fpu_thread = percpu_get(thread_t, fpu_thread);
	fpen(new == fpu_thread);
}

static kern_return_t fpu_alloc(pcb_t pcb)
{
	if (pcb->afs)
		return KERN_SUCCESS;

	pcb->afs = (struct aarch64_float_state *) kmem_cache_alloc(&fpu_cache);
	if (!pcb->afs)
		return KERN_RESOURCE_SHORTAGE;
	return KERN_SUCCESS;
}

static void fpu_save_state(void)
{
	thread_t	fpu_thread;
	pcb_t		pcb;

	fpu_thread = percpu_get(thread_t, fpu_thread);
	if (fpu_thread == THREAD_NULL)
		return;

	pcb = fpu_thread->pcb;
	fpu_alloc(pcb);

	_fpu_save_state(pcb->afs);
}

static void fpu_load_state(void)
{
	thread_t	thread = current_thread();
	pcb_t		pcb;

	pcb = thread->pcb;
	if (pcb->afs == NULL) {
		fpu_alloc(pcb);
		memset(pcb->afs, 0, sizeof(struct aarch64_float_state));
	}

	_fpu_load_state(pcb->afs);
	percpu_assign(fpu_thread, thread);
}

void fpu_access_trap(void)
{
	fpen(TRUE);
	fpu_save_state();
	fpu_load_state();
}

void fpu_flush_state_read(thread_t thread)
{
	thread_t	fpu_thread;

#if NCUPS > 1
#error "Implement"
#endif
	fpu_thread = percpu_get(thread_t, fpu_thread);
	if (thread != fpu_thread)
		return;

	fpen(TRUE);
	fpu_save_state();
	if (thread != current_thread())
		fpen(FALSE);
}

void fpu_flush_state_write(thread_t thread)
{
	thread_t	fpu_thread;

	fpu_alloc(thread->pcb);

#if NCUPS > 1
#error "Implement"
#endif
	fpu_thread = percpu_get(thread_t, fpu_thread);
	if (thread != fpu_thread)
		return;

	percpu_assign(fpu_thread, THREAD_NULL);
	fpen(FALSE);
}

void fpu_free(thread_t thread)
{
	thread_t	fpu_thread;

	fpu_thread = percpu_get(thread_t, fpu_thread);

	if (thread->pcb->afs == NULL) {
		assert(fpu_thread != thread);
		return;
	}
#if NCUPS > 1
#error "Implement"
#endif
	if (fpu_thread == thread)
		percpu_assign(fpu_thread, THREAD_NULL);
	kmem_cache_free(&fpu_cache, (vm_offset_t) thread->pcb->afs);
}
