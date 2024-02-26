#include "machine/trap.h"
#include "aarch64/model_dep.h"
#include "aarch64/locore.h"
#include <mach/exception.h>
#include <vm/vm_fault.h>
#include <kern/printf.h>
#include <kern/thread.h>
#include <kern/exception.h>

/* Extract exception class from ESR value.  */
#define ESR_EC(esr)		(((esr) >> 26) & 0x3f)
/* Exception classes.  */
#define ESR_EC_UNK		0x00		/* unknown reason */
#define ESR_EC_BTI		0x0d		/* BTI failure */
#define ESR_EC_ILL		0x0e		/* illegal execution state */
#define ESR_EC_SVC64		0x15		/* SCV (syscall) */
#define ESR_EC_PAC		0x1c		/* PAC failure */
#define ESR_EC_IABT_LOWER_EL	0x20		/* instruction abort from lower EL */
#define ESR_EC_IABT_SAME_EL	0x21		/* instruction abort from the same EL */
#define ESR_EC_AL_PC		0x22		/* misaligned PC */
#define ESR_EC_DABT_LOWER_EL	0x24		/* data abort from lower EL */
#define ESR_EC_DABT_SAME_EL	0x25		/* data abort from the same EL */
#define ESR_EC_AL_SP		0x26		/* misaligned SP */
#define ESR_EC_SERROR		0x2f		/* SError */
#define ESR_EC_BRK_LOWER_EL	0x30		/* breakpoint from lower EL */
#define ESR_EC_BRK_SAME_EL	0x31		/* breakpoint from the same EL */

#define ESR_IABT_IFSC(esr)	((esr) & 0x3f)	/* instruction fault status code */

#define ESR_IABT_IFSC_PERM_L0	0x0c		/* permission fault, level 0 */
#define ESR_IABT_IFSC_PERM_L1	0x0d		/* permission fault, level 1 */
#define ESR_IABT_IFSC_PERM_L2	0x0e		/* permission fault, level 2 */
#define ESR_IABT_IFSC_PERM_L3	0x0f		/* permission fault, level 3 */
#define ESR_IABT_IFSC_SYNC_EXT	0x10		/* synchronous external abort */

#define ESR_DABT_DFSC(esr)	((esr) & 0x3f)	/* data fault status code */

#define ESR_DABT_DFSC_MTE	0x11		/* synchronous MTE tag check fault */

#define ESR_DABT_WNR		0x40		/* "write, not read" bit */

#define ESR_ABT_FNV		0x400		/* "FAR not valid" bit (both IABT & DABT) */

static vm_prot_t esr_to_fault_type(unsigned long esr)
{
	switch (ESR_EC(esr)) {
		case ESR_EC_IABT_LOWER_EL:
			return VM_PROT_READ | VM_PROT_EXECUTE;
		case ESR_EC_DABT_LOWER_EL:
		case ESR_EC_DABT_SAME_EL:
			if (esr & ESR_DABT_WNR)
				return VM_PROT_READ | VM_PROT_WRITE;
			return VM_PROT_READ;
		default:
			panic("Unexpected exception class\n");
	}
}

static void user_page_fault_continue(kern_return_t kr)
{
	if (kr == KERN_SUCCESS)
		thread_exception_return();

	exception(EXC_BAD_ACCESS, kr, 0x1234);
}

void trap_aarch32(void)
{
	panic("Unexpected trap from AArch32 EL0\n");
}

void trap_irq_el0(void)
{
	printf("Got IRQ while in EL0, ignoring for now\n");
	thread_exception_return();
}

void trap_fiq_el0(void)
{
	printf("Got FIQ while in EL0, ignoring for now\n");
	thread_exception_return();
}

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
			exception(EXC_BAD_ACCESS, EXC_AARCH64_UNK, far);
		case ESR_EC_BTI:
			exception(EXC_BAD_ACCESS, EXC_AARCH64_BTI, far);
		case ESR_EC_ILL:
			exception(EXC_BAD_INSTRUCTION, EXC_AARCH64_ILL, far);
		case ESR_EC_SVC64:
			imm16 = esr & 0xf;
			/* "svc #0" is a syscall */
			if (imm16 == 0 && handle_syscall(&pcb->ats))
				thread_exception_return();
			exception(EXC_BAD_INSTRUCTION, EXC_AARCH64_SVC, 0);
		case ESR_EC_PAC:
			exception(EXC_BAD_ACCESS, EXC_AARCH64_PAC, far);
		case ESR_EC_IABT_LOWER_EL:
			if (ESR_IABT_IFSC(esr) >= ESR_IABT_IFSC_SYNC_EXT) {
				exception(EXC_BAD_INSTRUCTION, 0, 0);
			}
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
			(void) vm_fault(current_map(), trunc_page(far),
					esr_to_fault_type(esr),
					FALSE, FALSE,
					user_page_fault_continue);
			__builtin_unreachable();
		case ESR_EC_DABT_SAME_EL:
			panic("Same EL exception in EL0 handler\n");
		case ESR_EC_AL_SP:
			exception(EXC_BAD_ACCESS, EXC_AARCH64_AL_SP, far);
		case ESR_EC_SERROR:
			panic("SError in sync exc handler\n");
		case ESR_EC_BRK_LOWER_EL:
			exception(EXC_BREAKPOINT, EXC_AARCH64_BRK, far);
		case ESR_EC_BRK_SAME_EL:
			panic("Same EL exception in EL0 handler\n");
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
	printf("Got IRQ while in EL1, ignoring for now\n");
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

			kr = vm_fault(current_map(), trunc_page(far),
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
					akes->pc = recover_base + rp->recover_addr_off;
					return;
				}
			}

			panic("Kernel segfault\n");
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
