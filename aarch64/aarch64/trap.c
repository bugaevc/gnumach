#include "machine/trap.h"
#include "aarch64/model_dep.h"
#include <mach/exception.h>
#include <kern/printf.h>
#include <kern/thread.h>
#include <kern/exception.h>

static void user_page_fault_continue(kern_return_t kr)
{
	if (kr == KERN_SUCCESS)
		thread_exception_return();

	exception(EXC_BAD_ACCESS, kr, 0x1234);
}

void trap_aarch32(void)
{
	panic("Unexpected trap from AArch32 EL0");
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
	pcb_t	pcb = current_thread()->pcb;

	printf("Sync exc from EL0!\n");
	printf("ESR: %lx, FAR: %lx\n", pcb->esr, pcb->far);
	while (1) machine_idle(0);
}

void trap_serror_el0(void)
{
	panic("SError in EL0");
}

#if MACH_PCSAMPLE > 0
/*
 * return saved state for interrupted user thread
 */
unsigned
interrupted_pc(const thread_t t)
{
	struct aarch64_thread_state *ats;

	ats = USER_REGS(t);
	return ats->pc;
}
#endif  /* MACH_PCSAMPLE > 0 */
