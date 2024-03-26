#include "machine/trap.h"
#include "aarch64/model_dep.h"
#include "aarch64/locore.h"
#include "aarch64/fpu.h"
#include "aarch64/irq.h"
#include <mach/exception.h>
#include <vm/vm_fault.h>
#include <vm/vm_kern.h>
#include <kern/printf.h>
#include <kern/thread.h>
#include <kern/exception.h>

/* Extract exception class from ESR value.  */
#define ESR_EC(esr)		(((esr) >> 26) & 0x3f)
/* Exception classes.  */
#define ESR_EC_UNK		0x00		/* unknown reason */
#define ESR_EC_FP_ACCESS	0x07		/* FP/AdvSIMD access when disabled */
#define ESR_EC_BTI		0x0d		/* BTI failure */
#define ESR_EC_ILL		0x0e		/* illegal execution state */
#define ESR_EC_SVC64		0x15		/* SCV (syscall) */
#define ESR_EC_MRS		0x18		/* MRS or MRS (or cache?) */
#define ESR_EC_PAC		0x1c		/* PAC failure */
#define ESR_EC_IABT_LOWER_EL	0x20		/* instruction abort from lower EL */
#define ESR_EC_IABT_SAME_EL	0x21		/* instruction abort from the same EL */
#define ESR_EC_AL_PC		0x22		/* misaligned PC */
#define ESR_EC_DABT_LOWER_EL	0x24		/* data abort from lower EL */
#define ESR_EC_DABT_SAME_EL	0x25		/* data abort from the same EL */
#define ESR_EC_AL_SP		0x26		/* misaligned SP */
#define ESR_EC_FP_EXC		0x2c		/* FP exception */
#define ESR_EC_SERROR		0x2f		/* SError */
#define ESR_EC_BPT_LOWER_EL	0x30		/* breakpoint from lower EL */
#define ESR_EC_BPT_SAME_EL	0x31		/* breakpoint from the same EL */
#define ESR_EC_SSTEP_LOWER_EL	0x32		/* software single step from lower EL */
#define ESR_EC_SSTEP_SAME_EL	0x33		/* software single step from the same EL */
#define ESR_EC_BRK		0x3c		/* BRK */

#define ESR_IABT_IFSC(esr)	((esr) & 0x3f)	/* instruction fault status code */

#define ESR_IABT_IFSC_PERM_L0	0x0c		/* permission fault, level 0 */
#define ESR_IABT_IFSC_PERM_L1	0x0d		/* permission fault, level 1 */
#define ESR_IABT_IFSC_PERM_L2	0x0e		/* permission fault, level 2 */
#define ESR_IABT_IFSC_PERM_L3	0x0f		/* permission fault, level 3 */
#define ESR_IABT_IFSC_SYNC_EXT	0x10		/* synchronous external abort */

#define ESR_DABT_DFSC(esr)	((esr) & 0x3f)	/* data fault status code */

#define ESR_DABT_DFSC_MTE	0x11		/* synchronous MTE tag check fault */
#define ESR_DABT_DFSC_AL	0x21		/* alignment fault */

#define ESR_DABT_WNR		0x40		/* "write, not read" bit */

#define ESR_ABT_FNV		0x400		/* "FAR not valid" bit (both IABT & DABT) */

#define ESR_FP_EXC_TFV(esr)	((esr) & 0x800000) /* other FP bits hols meaningful values */
#define ESR_FP_EXC_IDF(esr)	((esr) & 0x80)	/* input denormal */
#define ESR_FP_EXC_IXF(esr)	((esr) & 0x10)	/* inexact */
#define ESR_FP_EXC_UFF(esr)	((esr) & 0x08)	/* underflow */
#define ESR_FP_EXC_OFF(esr)	((esr) & 0x04)	/* overflow */
#define ESR_FP_EXC_DZF(esr)	((esr) & 0x02)	/* divide by zero */
#define ESR_FP_EXC_IOF(esr)	((esr) & 0x01)	/* invalid operation */

#define ESR_BTI_BTYPE(esr)	((esr) & 0x3)	/* BTYPE that caused the BTI exception */

#define ESR_BRK_IMM(esr)	((esr) & 0xffff)

static vm_prot_t esr_to_fault_type(unsigned long esr)
{
	switch (ESR_EC(esr)) {
		case ESR_EC_IABT_LOWER_EL:
			return VM_PROT_EXECUTE;
		case ESR_EC_DABT_LOWER_EL:
		case ESR_EC_DABT_SAME_EL:
			if (esr & ESR_DABT_WNR)
				return VM_PROT_WRITE;
			return VM_PROT_READ;
		default:
			panic("Unexpected exception class\n");
	}
}

static void user_page_fault_continue(kern_return_t kr)
{
	pcb_t		pcb;

	if (likely(kr == KERN_SUCCESS))
		thread_exception_return();

	pcb = current_thread()->pcb;
	exception(EXC_BAD_ACCESS, kr, pcb->far);
}

void trap_aarch32(void)
{
	panic("Unexpected trap from AArch32 EL0\n");
}

void trap_irq_el0(void)
{
	spl7_irq();
	percpu_assign(in_irq_from_el0, TRUE);

	assert(root_irq_src);
	root_irq_src->handle_irq(root_irq_src);

	spl0_irq();
	percpu_assign(in_irq_from_el0, FALSE);

	thread_exception_return();
}

void trap_fiq_el0(void)
{
	printf("Got FIQ while in EL0, ignoring for now\n");
	thread_exception_return();
}

/*
 *	TODO: what should we do when FAR crosses a page boundary?
 */

void trap_sync_exc_el0(void)
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
			exception(EXC_BAD_INSTRUCTION, EXC_AARCH64_UNK, 0);
		case ESR_EC_FP_ACCESS:
			fpu_access_trap();
			thread_exception_return();
		case ESR_EC_BTI:
			exception(EXC_BAD_ACCESS, EXC_AARCH64_BTI, ESR_BTI_BTYPE(esr));
		case ESR_EC_ILL:
			exception(EXC_BAD_INSTRUCTION, EXC_AARCH64_ILL, far);
		case ESR_EC_SVC64:
			imm16 = esr & 0xf;
			/* "svc #0" is a syscall */
			if (likely(imm16 == 0) && handle_syscall(&pcb->ats))
				thread_exception_return();
			exception(EXC_BAD_INSTRUCTION, EXC_AARCH64_SVC, 0);
		case ESR_EC_MRS:
			/* We may add a special code for this (EXC_AARCH64_MRS?) */
			exception(EXC_BAD_INSTRUCTION, 0, 0);
		case ESR_EC_PAC:
			exception(EXC_BAD_ACCESS, EXC_AARCH64_PAC, far);
		case ESR_EC_IABT_LOWER_EL:
			if (ESR_IABT_IFSC(esr) >= ESR_IABT_IFSC_SYNC_EXT)
				exception(EXC_BAD_INSTRUCTION, 0, 0);
			if (far >= VM_MAX_USER_ADDRESS)
				exception(EXC_BAD_ACCESS, KERN_INVALID_ADDRESS, far);
			(void) vm_fault(current_map(), trunc_page(far),
					esr_to_fault_type(esr),
					FALSE, FALSE,
					user_page_fault_continue);
			__builtin_unreachable();
		case ESR_EC_IABT_SAME_EL:
			panic("Same EL exception in EL0 handler\n");
		case ESR_EC_AL_PC:
			exception(EXC_BAD_ACCESS, EXC_AARCH64_AL_PC, far);
		case ESR_EC_DABT_LOWER_EL:
			/* Data fault.  */
			if (far >= VM_MAX_USER_ADDRESS)
				exception(EXC_BAD_ACCESS, KERN_INVALID_ADDRESS, far);
			if (ESR_DABT_DFSC(esr) == ESR_DABT_DFSC_MTE)
				exception(EXC_BAD_ACCESS, EXC_AARCH64_MTE, far);
			if (ESR_DABT_DFSC(esr) == ESR_DABT_DFSC_AL)
				exception(EXC_BAD_ACCESS, EXC_AARCH64_AL, far);
			(void) vm_fault(current_map(), trunc_page(far),
					esr_to_fault_type(esr),
					FALSE, FALSE,
					user_page_fault_continue);
			__builtin_unreachable();
		case ESR_EC_DABT_SAME_EL:
			panic("Same EL exception in EL0 handler\n");
		case ESR_EC_AL_SP:
			/* Doesn't write to FAR.  */
			exception(EXC_BAD_ACCESS, EXC_AARCH64_AL_SP, pcb->ats.sp);
		case ESR_EC_FP_EXC:
			if (!ESR_FP_EXC_TFV(esr))
				exception(EXC_ARITHMETIC, 0, 0);
			/* Otherwise, we can look at what it is.  */
			if (ESR_FP_EXC_IDF(esr))
				exception(EXC_ARITHMETIC, EXC_AARCH64_IDF, 0);
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
		/*
		case ESR_EC_BPT_LOWER_EL:
			exception(EXC_BREAKPOINT, EXC_AARCH64_BRK, far);
		*/
		case ESR_EC_BPT_SAME_EL:
			panic("Same EL exception in EL0 handler\n");
		case ESR_EC_SSTEP_LOWER_EL:
			/*
			 *	TODO: need a userspace API to set/unset the MDSCR_EL1.SS bit,
			 *	perhaps as a part of aarch64_debug_state.
			 *	Unset it here before taking the exception.
			 *	https://lore.kernel.org/all/CAFEAcA8QmsHfxAdUQET2Oab_xXa7x4i4C4+_6Y-J8ZNs1t5pPg@mail.gmail.com/
			 */
			exception(EXC_BREAKPOINT, EXC_AARCH64_SSTEP, 0);
		case ESR_EC_SSTEP_SAME_EL:
			panic("Same EL exception in EL0 handler\n");
		case ESR_EC_BRK:
			exception(EXC_BREAKPOINT, EXC_AARCH64_BRK, ESR_BRK_IMM(esr));
		default:
			printf("Unhandled exception!\n");
			exception(EXC_BAD_INSTRUCTION, EXC_AARCH64_UNK, far);
	}
}

void trap_serror_el0(void)
{
	panic("SError in EL0\n");
}

void trap_irq_el1(void)
{
	spl7_irq();

	assert(root_irq_src);
	root_irq_src->handle_irq(root_irq_src);

	spl0_irq();
}

void trap_fiq_el1(void)
{
	printf("Got FIQ while in EL1, ignoring for now\n");
}

void trap_sync_exc_el1(
	unsigned long				esr,
	vm_offset_t				far,
	struct aarch64_kernel_exception_state	*akes)
{
	kern_return_t		kr;
	vm_map_t		map;
	const struct recovery	*rp;
	vm_offset_t		recover_base = (vm_offset_t) &recover_table;

#if 0
	printf("Sync exc from EL1!\n");
	printf("ESR: %#lx, FAR: %#lx, PC: %#lx\n", esr, far, akes->pc);
#endif

	switch (ESR_EC(esr)) {
		case ESR_EC_DABT_SAME_EL:
			/* Faulted on an address while in kernelspace.  */

			if (current_task() == kernel_task) {
				if (far <= PAGE_SIZE)
					panic("Kernel segfault at %p (NULL dereference?)\n", (void* ) far);
				else if (far <= VM_MAX_USER_ADDRESS)
					panic("Kernel thread accessed user space!\n");
			}

			if ((far >= VM_MIN_KERNEL_ADDRESS && far < kernel_virtual_start) || far >= kernel_virtual_end)
				panic("Kernel segfault at %p (physical memory area!)\n", (void *) far);

			if (far <= VM_MAX_USER_ADDRESS) {
				/*
				 *	Faulted on a user address.
				 *	This could be a PAN failure, or the fault
				 *	may be benign if there's a recovery handler
				 *	at this address.  We can detect would-be PAN
				 *	failures even if hardware PAN is not present
				 *	(SoftPAN).
				 */
				boolean_t	found = FALSE;
				for (rp = recover_table; rp < recover_table_end; rp++) {
					if ((vm_offset_t) akes->pc == recover_base + rp->fault_addr_off) {
						found = TRUE;
						break;
					}
				}
				if (!found)
					panic("SoftPAN failure at %p\n", (const void *) far);
				map = current_map();
			} else {
				map = kernel_map;
			}

			kr = vm_fault(map, trunc_page(far),
				      esr_to_fault_type(esr),
				      FALSE, FALSE, NULL);
			if (kr == KERN_SUCCESS)
				return;

			/*
			 *	If we have a recovery handler for this address,
			 *	jump there.
			 */
			for (rp = recover_table; rp < recover_table_end; rp++) {
				if ((vm_offset_t) akes->pc == recover_base + rp->fault_addr_off) {
					akes->pc = (void *) (recover_base + rp->recover_addr_off);
					/*
					 *	Set things up for an appropriate exception() call,
					 *	if that's what the handler wants to do.
					 */
					akes->x[0] = (long) EXC_BAD_ACCESS;
					akes->x[1] = (long) kr;
					akes->x[2] = (long) far;
					return;
				}
			}

			panic("Kernel segfault at %p\n", (void *) far);
		default:
			panic("Exception in EL1\n");
	}
}

void trap_serror_el1(void)
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
