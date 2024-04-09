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

#include "machine/trap.h"
#include "aarch64/locore.h"
#include "aarch64/fpu.h"
#include "aarch64/irq.h"
#include "aarch64/bits/esr.h"
#include <mach/exception.h>
#include <vm/vm_fault.h>
#include <vm/vm_kern.h>
#include <kern/printf.h>
#include <kern/thread.h>
#include <kern/exception.h>

static inline vm_prot_t fault_prot(unsigned long esr)
{
	/* Instruction aborts only need execute permission.  */
	if (ESR_EC(esr) == ESR_EC_IABT_LOWER_EL)
		return VM_PROT_EXECUTE;

	assert(ESR_EC(esr) == ESR_EC_DABT_LOWER_EL
	       || ESR_EC(esr) == ESR_EC_DABT_SAME_EL
	       || ESR_EC(esr) == ESR_EC_WATCHPT_LOWER_EL
	       || ESR_EC(esr) == ESR_EC_WATCHPT_SAME_EL);

	/* If WNR is unset, it's a read fault.  */
	if (!(esr & ESR_WNR))
		return VM_PROT_READ;

	/*
	 *	WNR is set if this was either an actual write fault,
	 *	or a cache maintenance operation.  So if it's not one,
	 *	which is the common case, it's a write fault.
	 */
	if (likely(!(esr & ESR_CM)))
		return VM_PROT_WRITE;

	/*
	 *	For cache maintenance operations, check if DFSC
	 *	indicates a permission fault, and only treat it as
	 *	a write fault in that case.  For watchpoints, we can't
	 *	exactly know, so let's guess it's a write fault.
	 *
	 *	Note that mistaking a read fault for a write fault is
	 *	likely to be fatal for data aborts, and likely to not
	 *	matter much for watchpoints.
	 */
	if ((ESR_DABT_DFSC(esr) >= ESR_DABT_DFSC_PERM_L0
	     && ESR_DABT_DFSC(esr) <= ESR_DABT_DFSC_PERM_L3)
	    || ESR_DABT_DFSC(esr) == ESR_WATCHPT_DFSC)
		return VM_PROT_WRITE;
	return VM_PROT_READ;
}

static void user_page_fault_continue(kern_return_t kr)
{
	pcb_t		pcb;

	if (likely(kr == KERN_SUCCESS))
		thread_exception_return();

	pcb = current_thread()->pcb;
	exception(EXC_BAD_ACCESS, kr, pcb->far);
}

void user_trap_aarch32(void)
{
	panic("Unexpected trap from AArch32 EL0\n");
}

void user_trap_irq(void)
{
	spl7_irq();
	percpu_assign(in_irq_from_el0, TRUE);

	assert(root_irq_src);
	root_irq_src->handle_irq(root_irq_src);

	spl0_irq();
	percpu_assign(in_irq_from_el0, FALSE);

	thread_exception_return();
}

void user_trap_fiq(void)
{
	printf("Got FIQ while in EL0, ignoring for now\n");
	thread_exception_return();
}

/*
 *	TODO: what should we do when FAR crosses a page boundary?
 *	Can this happen, or do unaligned accesses get reporter as
 *	two separate ones?
 */

/*
 *	Routine:	user_trap_sync
 *	Purpose:
 *		Handle a synchronous exception from EL0.
 *	Conditions:
 *		Exceptions are unmasked.  Nothing is locked.  User's
 *		general-purpose registers have been saved into the PCB,
 *		along with the ESR and FAR.
 *	Returns:
 *		Doesn't return, must exit via exception() or
 *		thread_exception_return() and similar.
 */
void user_trap_sync(void)
{
	pcb_t		pcb = current_thread()->pcb;
	unsigned long	esr = pcb->esr;
	vm_offset_t	far = pcb->far;
	int		imm16;

#if 0
	printf("Sync exc from EL0!\n");
	printf("ESR: %#lx, FAR: %#lx\n", esr, far);
#endif

	switch (ESR_EC(esr)) {
		case ESR_EC_UNK:
			/*
			 *	Unknown (unallocated) instruction; or one of
			 *	the many misc causes.  "This EC code is used
			 *	for all exceptions that are not covered by
			 *	any other EC value."
			 *
			 *	We get absolutely no additional information
			 *	encoded in ESR (or FAR), so there isn't really
			 *	much we can do here (short of fetching and
			 *	decoding the culprit instruction).  So just
			 *	report it to user and hope they can figure
			 *	out what went wrong.
			 */
			exception(EXC_BAD_INSTRUCTION, EXC_AARCH64_UNK, 0);

		case ESR_EC_WF:
			/*
			 *	WFE, WFI, WFET, and WFIT instructions.  When
			 *	executed at EL1, these put the CPU into a low-
			 *	power state until an exception happens.
			 *
			 *	Emulate the same semantics for the user by
			 *	blocking the thread until something aborts it.
			 *	This is primarily meant for VMs, since they
			 *	can't block by performing explicit syscalls,
			 *	and do expect WF* to put the virtual CPU into
			 *	a low-power state.  We also allow it for normal
			 *	tasks.
			 *
			 *	Unlike for SVC & friends, we must advance PC
			 *	explicitly.  Do this first, before we go to
			 *	sleep, so it looks like we're blocking on the
			 *	next instruction, same as for SVC.
			 *
			 *	We don't currently support WFxT properly (it's
			 *	unclear which timer it should work on), so for
			 *	them just make a single attempt to switch to
			 *	another thread, and return without waiting
			 *	otherwise.
			 */
			pcb->ats.pc += 4;

			switch (ESR_WF_TI(esr)) {
				case ESR_WF_TI_WFI:
					thread_will_wait(current_thread());
					thread_block(thread_exception_return);
					__builtin_unreachable();

				case ESR_WF_TI_WFE:
				case ESR_WF_TI_WFIT:
				case ESR_WF_TI_WFET:
				default:
					// TODO: thread_will_wait_with_timeout
					thread_block(thread_exception_return);
					__builtin_unreachable();
			}

		case ESR_EC_FP_ACCESS:
			/*
			 *	Userland accessed floating point registers
			 *	(typically for SIMD) while floating-point
			 *	access was trapped, because other thread's
			 *	FP state is loaded in the FP registers.
			 *
			 *	Take the appropriate action to allow the
			 *	current thread to use FP registers, and
			 *	continue as if nothing has happened.
			 */
			fpu_access_trap();
			thread_exception_return();

		case ESR_EC_BTI:
			/*
			 *	Branch target indentification failure: an
			 *	indirect branch to an instruction that wasn't
			 *	intended to be a target of indirect branches.
			 *
			 *	We treat it much like a branch to a non-
			 *	executable memory region, so file it under
			 *	EXC_BAD_ACCESS.  glibc maps this to SIGILL
			 *	however.
			 *
			 *	We get BTYPE (indirect branch type) indicated
			 *	in ESR, so pass that to user.
			 */
			exception(EXC_BAD_ACCESS, EXC_AARCH64_BTI, ESR_BTI_BTYPE(esr));

		case ESR_EC_IL:
			/*
			 *	Illegal execution state.
			 *
			 *	We would get one of these *from EL1* if we
			 *	attempt to return from an exception with a bad
			 *	execution state indicated in SPSR_EL1.
			 *
			 *	The only way to get one of these *from EL0* is
			 *	to ask for it explicitly by setting the IL flag
			 *	in PSTATE (CPSR) with a thread_set_state() call
			 *	(see "Legal returns that set PSTATE.IL to 1").
			 */
			exception(EXC_BAD_INSTRUCTION, EXC_AARCH64_IL, 0);

		case ESR_EC_SVC:
			/*
			 *	The "SVC" (syscall) instruction, with a 16-bit
			 *	immediate argument.  Note that ELR (ats.pc)
			 *	gets pointed to the following instruction by
			 *	hardware, so simply returning from the
			 *	exception resumes execution after the sycall.
			 *
			 *	We recognize "SVC #0" with a valid negated trap
			 *	index (in w8) as a Mach trap, and report a
			 *	dedicated exception otherwise.  This could be
			 *	used by a user-level exception handler to
			 *	implement foreign syscall emulation.
			 *
			 *	Note that handle_syscall() doesn't necesserily
			 *	return normally; some syscalls return to user
			 *	by themselves.
			 */
			imm16 = ESR_SVC_IMM(esr);
			if (likely(imm16 == 0) && handle_syscall(&pcb->ats))
				thread_exception_return();
			exception(EXC_SOFTWARE, EXC_AARCH64_SVC, imm16);

		case ESR_EC_HVC:
			/*
			 *	The "HVC" (hypervisor call) instruction, very
			 *	similar to SVC.
			 *
			 *	We always report these as exceptions; a
			 *	hypervisor running as a user task should know
			 *	what to do with them.
			 */
#ifdef notyet
			imm16 = ESR_HVC_IMM(esr);
			exception(EXC_SOFTWARE, EXC_AARCH64_HVC, imm16);
#else
			panic("Virtualization not supported yet\n");
#endif

		case ESR_EC_SMC:
			/*
			 *	The "SMC" (secure monitor call) instruction,
			 *	very similar to SVC/SMC.
			 */
#ifdef notyet
			imm16 = ESR_SMC_IMM(esr);
			exception(EXC_SOFTWARE, EXC_AARCH64_SMC, imm16);
#else
			panic("Virtualization not supported yet\n");
#endif

		case ESR_EC_MRS:
			/* We may add a special code for this (EXC_AARCH64_MRS?) */
			exception(EXC_BAD_INSTRUCTION, 0, 0);

		case ESR_EC_PAC:
			/*
			 *	Pointer authentication failure.  This is only
			 *	generated when we have FEAT_FPAC, otherwise an
			 *	authentication failure simply results in an
			 *	intentionally invalid pointer, which will then
			 *	trap normally if dereferenced.
			 *
			 *	We treat this like a bad pointer dereference
			 *	(which it will be without FEAT_FPAC), but with
			 *	a special code indicating its PAC nature.  We
			 *	get two bits of information (whether A or B and
			 *	whether instruction or data key has been used),
			 *	which we pass on to the user.
			 */
			exception(EXC_BAD_ACCESS, EXC_AARCH64_PAC, ESR_PAC_INFO(esr));

		case ESR_EC_IABT_LOWER_EL:
			/*
			 *	Instruction abort: invalid address, non-
			 *	executable region, or something of that sort.
			 */
			if (ESR_IABT_IFSC(esr) >= ESR_IABT_IFSC_SYNC_EXT)
				exception(EXC_BAD_INSTRUCTION, 0, 0);		/* huh? */
			/*
			 *	If this is not a valid user address, do not
			 *	even bother trying to resolve it.  There's
			 *	nothing there as far as user is concerned.
			 */
			if (unlikely(far >= VM_MAX_USER_ADDRESS))
				exception(EXC_BAD_ACCESS, KERN_INVALID_ADDRESS, far);

			/*
			 *	Resolve the fault against the user's map.
			 *	We pass user_page_fault_continue() as the
			 *	continuation, so this call never returns.
			 */
			(void) vm_fault(current_map(), trunc_page(far),
					fault_prot(esr),
					FALSE, FALSE,
					user_page_fault_continue);
			__builtin_unreachable();

		case ESR_EC_IABT_SAME_EL:
			panic("Same EL exception in EL0 handler\n");

		case ESR_EC_AL_PC:
			/*
			 *	Misaligned PC.  FAR and ats.pc both hold the
			 *	misaligned PC value.
			 */
			exception(EXC_BAD_ACCESS, EXC_AARCH64_AL_PC, far);

		case ESR_EC_DABT_LOWER_EL:
			/*
			 *	Data fault.
			 */

			/*
			 *	If this is not a valid user address, do not
			 *	even bother trying to resolve it.  There's
			 *	nothing there as far as user is concerned.
			 */
			if (unlikely(far >= VM_MAX_USER_ADDRESS))
				exception(EXC_BAD_ACCESS, KERN_INVALID_ADDRESS, far);

			if (ESR_DABT_DFSC(esr) == ESR_DABT_DFSC_MTE)
#ifdef notyet
				exception(EXC_BAD_ACCESS, EXC_AARCH64_MTE, far);
#else
				panic("MTE is not supported yet\n");
#endif

			if (ESR_DABT_DFSC(esr) == ESR_DABT_DFSC_AL)
				exception(EXC_BAD_ACCESS, EXC_AARCH64_AL, far);

			/*
			 *	Resolve the fault against the user's map.
			 *	We pass user_page_fault_continue() as the
			 *	continuation, so this call never returns.
			 */
			(void) vm_fault(current_map(), trunc_page(far),
					fault_prot(esr),
					FALSE, FALSE,
					user_page_fault_continue);
			__builtin_unreachable();

		case ESR_EC_DABT_SAME_EL:
			panic("Same EL exception in EL0 handler\n");

		case ESR_EC_AL_SP:
			/*
			 *	SP was not 16-aligned on an SP-relative memory
			 *	access attempt.  QEMU doesn't report this.
			 *
			 *	This doesn't set FAR.
			 */
			exception(EXC_BAD_ACCESS, EXC_AARCH64_AL_SP, pcb->ats.sp);

		case ESR_EC_FP_EXC:
			/*
			 *	Some sort of floating-point exception, but not
			 *	an attempt to access floating-point registers
			 *	when trapped (see the ESR_EC_FP_ACCESS case for
			 *	that), and not missing floating-point support
			 *	altogether (that gets reported as ESR_EC_UNK).
			 *
			 *	There are a bunch of flags that indicate what
			 *	exactly has happened, but we can only look at
			 *	them if TFV is set.
			 */
			if (!ESR_FP_EXC_TFV(esr))
				exception(EXC_ARITHMETIC, 0, 0);

			if (ESR_FP_EXC_IDF(esr))
				exception(EXC_ARITHMETIC, EXC_AARCH64_IDF, 0);
			if (ESR_FP_EXC_IXF(esr))
				exception(EXC_ARITHMETIC, EXC_AARCH64_IXF, 0);
			if (ESR_FP_EXC_UFF(esr))
				exception(EXC_ARITHMETIC, EXC_AARCH64_UFF, 0);
			if (ESR_FP_EXC_OFF(esr))
				exception(EXC_ARITHMETIC, EXC_AARCH64_OFF, 0);
			if (ESR_FP_EXC_DZF(esr))
				exception(EXC_ARITHMETIC, EXC_AARCH64_DZF, 0);
			if (ESR_FP_EXC_IOF(esr))
				exception(EXC_ARITHMETIC, EXC_AARCH64_IOF, 0);
			/* Huh? */
			exception(EXC_ARITHMETIC, 0, 0);

		case ESR_EC_SERROR:
			panic("SError in sync exc handler\n");

		case ESR_EC_BREAKPT_LOWER_EL:
			/*
			 *	A hardware breakpoint triggered.  We don't get
			 *	any more details, but the user should be able
			 *	to match the faulting PC to a previously set
			 *	breakpoint.
			 */
			exception(EXC_BREAKPOINT, EXC_AARCH64_BREAKPT, 0);

		case ESR_EC_BREAKPT_SAME_EL:
			panic("Same EL exception in EL0 handler\n");

		case ESR_EC_SS_LOWER_EL:
			/*
			 *	TODO: need a userspace API to set/unset the MDSCR_EL1.SS bit,
			 *	perhaps as a part of aarch64_debug_state.
			 *	Unset it here before taking the exception.
			 *	https://lore.kernel.org/all/CAFEAcA8QmsHfxAdUQET2Oab_xXa7x4i4C4+_6Y-J8ZNs1t5pPg@mail.gmail.com/
			 */
			exception(EXC_BREAKPOINT, EXC_AARCH64_SS,
				  ESR_SS_ISV(esr) ? -1L : !!ESR_SS_EX(esr));

		case ESR_EC_SS_SAME_EL:
			panic("Same EL exception in EL0 handler\n");

		case ESR_EC_WATCHPT_LOWER_EL:
#ifdef notyet
			exception(EXC_BREAKPOINT,
				  (fault_prot(esr) == VM_PROT_READ) ?
				  EXC_AARCH64_WATCHPT_READ : EXC_AARCH64_WATCHPT_WRITE,
				  far);
#else
			panic("Watchpoints not supported yet\n");
#endif

		case ESR_EC_WATCHPT_SAME_EL:
			panic("Same EL exception in EL0 handler\n");

		case ESR_EC_BRK:
			/*
			 *	The "BRK" (software breakpoint) instruction,
			 *	with a 16-bit immediate argument.
			 */
			exception(EXC_BREAKPOINT, EXC_AARCH64_BRK, ESR_BRK_IMM(esr));

		default:
			printf("Unhandled exception! esr = %lx\n", esr);
			exception(EXC_BAD_INSTRUCTION, EXC_AARCH64_UNK, far);
	}
}

void user_trap_serror(void)
{
	panic("SError in EL0\n");
}

void kernel_trap_irq(void)
{
	spl7_irq();

	assert(root_irq_src);
	root_irq_src->handle_irq(root_irq_src);

	spl0_irq();
}

void kernel_trap_fiq(void)
{
	printf("Got FIQ while in EL1, ignoring for now\n");
}

/*
 *	Routine:	look_up_recovery
 *	Purpose:
 *		Look up the recovery address for a kernel fault.
 *	Returns:
 *		NULL	no recovery.
 *		addr	recovery address.
 */
static void *look_up_recovery(void *pc) {
	const struct recovery	*rp;
	vm_offset_t		recover_base = (vm_offset_t) &recover_table;

	for (rp = recover_table; rp < recover_table_end; rp++) {
		if ((vm_offset_t) pc == recover_base + rp->fault_addr_off) {
			return (void *) (recover_base + rp->recover_addr_off);
		}
	}

	return NULL;
}

/*
 *	Routine:	kernel_trap_sync
 *	Purpose:
 *		Handle a synchronous exception from EL1.
 *	Conditions:
 *		Exceptions are masked.  Nothing is locked.  Caller-saved
 *		registers have been saved, and their values can be
 *		modified by this routine to return to a different state.
 *	Returns:
 *		TRUE	if the exception has been handled, and execution
 *			should continue from where it got interrupted.
 *		FALSE	if the exception is fatal, and execution should
 *			abort after dumping registers.
 *	Parameters:
 *		esr	exception syndrome register
 *		far	fault address register
 *		akes	caller-saved registers at the time of the fault
 */
boolean_t kernel_trap_sync(
	unsigned long				esr,
	vm_offset_t				far,
	struct aarch64_kernel_exception_state	*akes)
{
	unsigned short	ec = ESR_EC(esr);
	kern_return_t	kr;
	vm_map_t	map;
	void		*recovery = NULL;

#if 0
	printf("Sync exc from EL1!\n");
	printf("ESR: %#lx, FAR: %#lx, PC: %#lx\n", esr, far, akes->pc);
#endif

#if MACH_KDB
	/*
	 *	First thing, unmask debug exceptions, unless this is
	 *	a debug exception that we're handling.
	 */
	switch (ec) {
		case ESR_EC_BREAKPT_SAME_EL:
		case ESR_EC_SS_SAME_EL:
		case ESR_EC_WATCHPT_SAME_EL:
		case ESR_EC_BRK:
			break;
		default:
			asm volatile("msr DAIFClr, #8");
			break;
	}
#endif

	switch (ec) {
		case ESR_EC_DABT_SAME_EL:
			/* Faulted on an address while in kernelspace.  */

			if ((far >= VM_MIN_KERNEL_ADDRESS && far < kernel_virtual_start)
			    || far >= kernel_virtual_end) {
				printf("Kernel segfault in physical memory area!\n");
				return FALSE;
			}

			if (far <= VM_MAX_USER_ADDRESS) {
				/*
				 *	Faulted on a user address.
				 *	This could be a PAN failure, or the fault
				 *	may be benign if there's a recovery handler
				 *	at this address.  We can detect would-be PAN
				 *	failures even if hardware PAN is not present
				 *	(SoftPAN).
				 */
				if (current_task() != kernel_task)
					recovery = look_up_recovery(akes->pc);
				if (unlikely(!recovery)) {
					printf("SoftPAN failure\n");
					return FALSE;
				}
				map = current_map();
			} else {
				map = kernel_map;
			}

			kr = vm_fault(map, trunc_page(far),
				      fault_prot(esr),
				      FALSE, FALSE, NULL);
			if (likely(kr == KERN_SUCCESS))
				return TRUE;

			/*
			 *	If we have a recovery handler for this address,
			 *	jump there.
			 */

			if (likely(recovery)) {
				/*
				 *	Set things up for an appropriate exception() call,
				 *	if that's what the handler wants to do.
				 */
				akes->pc = recovery;
				akes->x[0] = (long) EXC_BAD_ACCESS;
				akes->x[1] = (long) kr;
				akes->x[2] = (long) far;
				return TRUE;
			}

			printf("Kernel segfault\n");
			return FALSE;

		default:
			printf("Unexpected exception in EL1\n");
			return FALSE;
	}
}

void kernel_trap_fatal(
	unsigned long			esr,
	vm_offset_t			far,
	struct aarch64_thread_state 	*ats)
{
	printf("=====================\n");
	printf("Dump of kernel state:\n");
	printf(" x0 %016lx  x1 %016lx  x2 %016lx  x3 %016lx\n",
	       ats->x[0], ats->x[1],
	       ats->x[2], ats->x[3]);
	printf(" x4 %016lx  x5 %016lx  x6 %016lx  x7 %016lx\n",
	       ats->x[4], ats->x[5],
	       ats->x[6], ats->x[7]);
	printf(" x8 %016lx  x9 %016lx x10 %016lx x11 %016lx\n",
	       ats->x[8], ats->x[9],
	       ats->x[10], ats->x[11]);
	printf("x12 %016lx x13 %016lx x14 %016lx x15 %016lx\n",
	       ats->x[12], ats->x[13],
	       ats->x[14], ats->x[15]);
	printf("x16 %016lx x17 %016lx x18 %016lx x19 %016lx\n",
	       ats->x[16], ats->x[17],
	       ats->x[18], ats->x[19]);
	printf("x20 %016lx x21 %016lx x22 %016lx x23 %016lx\n",
	       ats->x[20], ats->x[21],
	       ats->x[22], ats->x[23]);
	printf("x24 %016lx x25 %016lx x26 %016lx x27 %016lx\n",
	       ats->x[24], ats->x[25],
	       ats->x[26], ats->x[27]);
	printf("x28 %016lx x29 %016lx x30 %016lx  sp %016lx\n",
	       ats->x[28], ats->x[29],
	       ats->x[30], ats->sp);
	printf(" pc %016lx psr %016lx esr %016lx far %016lx\n",
	       ats->pc, ats->cpsr,
	       esr, far);

	panic("Fatal exception");
}

void kernel_trap_serror(void)
{
	panic("SError in EL1\n");
}

#if MACH_PCSAMPLE > 0
/*
 *	Return saved state for interrupted user thread.
 */
unsigned interrupted_pc(const thread_t t)
{
	return USER_REGS(t)->pc;
}
#endif  /* MACH_PCSAMPLE > 0 */
