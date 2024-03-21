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

#ifndef _AARCH64_THREAD_
#define _AARCH64_THREAD_

#include "aarch64/vm_param.h"
#include "mach/machine/thread_status.h"

/*
 *	The state of a blocked kernel thread, see Switch_context().
 *	Since blocking looks like a function call, we only have to
 *	save callee-saved registers.
 */
struct aarch64_kernel_state {
	long	k_regs[12];	/* x19 to x30 */
	long	k_sp;
};
#define AKS_REG(aks, reg)		((aks)->k_regs[(reg) - 19])

/*
 *	The state saved on the stack when taking an exception from
 *	EL1.  We only have to save the caller-saved registers.
 */
struct aarch64_kernel_exception_state {
	long	x[19];		/* x0 to x18 */
	void	*x30;
	void	*pc;
	long	cpsr;
};

typedef struct pcb {
	struct aarch64_float_state *afs;
	struct aarch64_thread_state ats;
	long	esr;
	long	far;
} *pcb_t;

#define USER_REGS(thread)		(&(thread)->pcb->ats)

struct aarch64_exception_link {
	/* points to pcb->ats */
	struct aarch64_thread_state *saved_state;
};

#define STACK_AKS(stack)	\
	((struct aarch64_kernel_state *)((stack) + KERNEL_STACK_SIZE) - 1)
#define STACK_AEL(stack)	\
	((struct aarch64_exception_link *)STACK_AKS(stack) - 1)
#define STACK_AKS_REG(stack, reg)	AKS_REG(STACK_AKS(stack), reg)

#endif /* _AARCH64_THREAD_ */
