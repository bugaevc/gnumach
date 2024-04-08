#include "pcb.h"
#include "aarch64/vm_param.h"
#include "aarch64/pmap.h"
#include "aarch64/fpu.h"
#include "aarch64/hwcaps.h"
#include "aarch64/bits/spsr.h"
#include <vm/vm_map.h>
#include <kern/slab.h>
#include <kern/counters.h>
#include <string.h>

/* Top of active stack (high address).  */
vm_offset_t	kernel_stack[NCPUS];

struct kmem_cache pcb_cache;

void pcb_module_init(void)
{
	kmem_cache_init(&pcb_cache, "pcb", sizeof(struct pcb),
			alignof(struct pcb), NULL, 0);
}

void stack_attach(
	thread_t	thread,
	vm_offset_t	stack,
	void		(*continuation)(thread_t))
{
	counter(if (++c_stacks_current > c_stacks_max)
		c_stacks_max = c_stacks_current);

	assert(thread->kernel_stack == 0);
	thread->kernel_stack = stack;
	STACK_AKS_REG(stack, 30) = (long) Thread_continue;
	STACK_AKS_REG(stack, 19) = (long) continuation;
	STACK_AKS_REG(stack, 29) = (long) 0;
	STACK_AKS(stack)->k_sp = (long) STACK_AEL(stack);

	STACK_AEL(stack)->saved_state = USER_REGS(thread);
}

vm_offset_t stack_detach(thread_t thread)
{
	vm_offset_t	stack;

	counter(if (--c_stacks_current < c_stacks_min)
		c_stacks_min = c_stacks_current;)

	stack = thread->kernel_stack;
	thread->kernel_stack = 0;

	return stack;
}

void stack_handoff(
	thread_t	old,
	thread_t	new)
{
	task_t		old_task, new_task;
	int		mycpu = cpu_number();
	vm_offset_t	stack;

	old_task = old->task;
	new_task = new->task;
	if (old_task != new_task) {
		PMAP_DEACTIVATE_USER(vm_map_pmap(old_task->map), old, mycpu);
		PMAP_ACTIVATE_USER(vm_map_pmap(new_task->map), new, mycpu);
	}

	fpu_switch_context(new);

	assert(new->kernel_stack == 0);
	stack = current_stack();
	old->kernel_stack = 0;
	new->kernel_stack = stack;

	percpu_assign(active_thread, new);

	STACK_AEL(stack)->saved_state = USER_REGS(new);
}

extern thread_t Switch_context(thread_t old, continuation_t continuation, thread_t new);

thread_t switch_context(
	thread_t	old,
	continuation_t	continuation,
	thread_t	new)
{
	task_t	old_task, new_task;
	int	mycpu = cpu_number();

	old_task = old->task;
	new_task = new->task;
	if (old_task != new_task) {
		PMAP_DEACTIVATE_USER(vm_map_pmap(old_task->map), old, mycpu);
		PMAP_ACTIVATE_USER(vm_map_pmap(new_task->map), new, mycpu);
	}

	fpu_switch_context(new);

	return Switch_context(old, continuation, new);
}

void pcb_init(task_t parent_task, thread_t thread)
{
	pcb_t	pcb;

	if (parent_task == kernel_task) {
		thread->pcb = NULL;
		return;
	}

	pcb = (pcb_t) kmem_cache_alloc(&pcb_cache);
	if (pcb == NULL)
		panic("pcb_init");

	counter(if (++c_threads_current > c_threads_max)
		c_threads_max = c_threads_current);

	memset(pcb, 0, sizeof(*pcb));

	thread->pcb = pcb;
}

void pcb_terminate(thread_t thread)
{
	counter(if (--c_threads_current < c_threads_min)
		c_threads_min = c_threads_current);

	fpu_free(thread);
	kmem_cache_free(&pcb_cache, (vm_offset_t) thread->pcb);
	thread->pcb = NULL;
}

void pcb_collect(__attribute__((unused)) const thread_t thread)
{
}

void thread_set_syscall_return(
	thread_t	thread,
	kern_return_t	kr)
{
	USER_REGS(thread)->x[0] = kr;
}

kern_return_t thread_getstatus(
	thread_t	thread,
	int		flavor,
	thread_state_t	tstate,
	unsigned int	*count)
{
	switch (flavor) {
		case THREAD_STATE_FLAVOR_LIST:
			if (*count < 2)
				return KERN_INVALID_ARGUMENT;
			tstate[0] = AARCH64_THREAD_STATE;
			tstate[1] = AARCH64_FLOAT_STATE;
			*count = 2;
			return KERN_SUCCESS;

		case AARCH64_THREAD_STATE:
			if (*count < AARCH64_THREAD_STATE_COUNT)
				return KERN_INVALID_ARGUMENT;
			memcpy(tstate, USER_REGS(thread), sizeof(struct aarch64_thread_state));
			*count = AARCH64_THREAD_STATE_COUNT;
			return KERN_SUCCESS;

		case AARCH64_FLOAT_STATE:
			if (*count < AARCH64_FLOAT_STATE_COUNT)
				return KERN_INVALID_ARGUMENT;
			fpu_flush_state_read(thread);
			if (thread->pcb->afs == NULL)
				memset(tstate, 0, sizeof(struct aarch64_float_state));
			else
				memcpy(tstate, thread->pcb->afs, sizeof(struct aarch64_float_state));
			*count = AARCH64_FLOAT_STATE_COUNT;
			return KERN_SUCCESS;

		default:
			return KERN_INVALID_ARGUMENT;
	}
}

/*
 *	validate_cpsr:
 *
 *	Check the CPSR the user is trying to set for
 *	any disallowed/privileged/reserved bits.
 */
static boolean_t validate_cpsr(long cpsr, long old_cpsr)
{
	long		res0 = SPSR_RES0;

	/*
	 *	Make sure the CPSR indicates a valid state,
	 *	specifically EL0 AArch64.
	 *
	 *	Note that both SPSR_EL() = 0 and SPSR_NRW_64
	 *	have a zero bit pattern, so just initializing
	 *	CPSR to 0 should pass the checks successfully.
	 */
	if (SPSR_EL(cpsr) != 0)
		return FALSE;
	if (SPSR_NRW(cpsr) != SPSR_NRW_64)
		return FALSE;

	/*
	 *	Let userland mask debug exceptions if they
	 *	so want, but not IRQs, FIQs, or SErrors.
	 *
	 *	TODO: ARM ARM seems to say D is ignored at EL0.
	 */
	if (cpsr & SPSR_AIF)
		return FALSE;
	if (cpsr & SPSR_ALLINT)
		return FALSE;

	/*
	 *	SPSR_BTYPE:
	 *		OK to set if we have HWCAP2_BTI.
	 *	SPSR_SSBS:
	 *		OK to set if we have HWCAP_SSBS.
	 *		Resets to 0 or 1 on exception entry
	 *		according to SCTLR_SSBS.
	 *	SPSR_IL:
	 *		OK (and fun) to set.
	 *		Will cause an immediate EXC_AARCH64_IL.
	 *	SPSR_SS:
	 *		OK to set.
	 *	SPSR_PAN:
	 *		OK to set if we have HWCAP_INT_PAN.
	 *		Doesn't affect EL0.
	 *		Resets to 1 (given SCTLR_SPAN is unset)
	 *		on exception entry.
	 *	SPSR_UAO:
	 *		OK to set if we have HWCAP_INT_UAO.
	 *		Doesn't affect EL0.
	 *		Resets to 0 on exception entry.
	 *	SPSR_DIT:
	 *		OK to set if we have HWCAP_DIT.
	 *	SPSR_TCO:
	 *		OK to set if we have HWCAP2_MTE.
	 *	SPSR_NZCV:
	 *		OK to set.
	 */
	if (!(hwcaps[1] & HWCAP2_BTI))
		res0 |= SPSR_BTYPE_MASK;
	if (!(hwcaps[0] & HWCAP_SSBS))
		res0 |= SPSR_SSBS;
	if (!(hwcap_internal & HWCAP_INT_PAN))
		res0 |= SPSR_PAN;
	if (!(hwcap_internal & HWCAP_INT_UAO))
		res0 |= SPSR_UAO;
	if (!(hwcaps[0] & HWCAP_DIT))
		res0 |= SPSR_DIT;
	if (!(hwcaps[1] & HWCAP2_MTE))
		res0 |= SPSR_TCO;

	/*
	 *	Allow setting reserved bits to either 0 or
	 *	the value it already had.  In other words,
	 *	disallow setting any new reserved bits.
	 */
	if (cpsr & res0 & ~old_cpsr)
		return FALSE;

	return TRUE;
}

static boolean_t validate_fpcr(long fpcr, long old_fpcr) {
	/* TODO */
	return TRUE;
}

static boolean_t validate_fpsr(long fpsr, long old_fpsr) {
	/* TODO */
	return TRUE;
}

static boolean_t validate_fpmr(long fpmr, long old_fpmr) {
	/* TODO: support FPMR */
	return fpmr == 0 || fpmr == old_fpmr;
}

#define old_fpr(thread, reg)		((thread)->pcb->afs ? (thread)->pcb->afs->reg : 0)

kern_return_t thread_setstatus(
	thread_t	thread,
	int		flavor,
	thread_state_t	tstate,
	unsigned int	count)
{
	struct aarch64_thread_state	*ats;
	struct aarch64_float_state	*afs;

	switch (flavor) {
		case AARCH64_THREAD_STATE:
			if (count < AARCH64_THREAD_STATE_COUNT)
				return KERN_INVALID_ARGUMENT;
			if (((vm_offset_t) tstate) % alignof(struct aarch64_thread_state))
				return KERN_INVALID_ARGUMENT;
			ats = (struct aarch64_thread_state *) tstate;

			if (!validate_cpsr(ats->cpsr, USER_REGS(thread)->cpsr))
				return KERN_INVALID_ARGUMENT;

			memcpy(USER_REGS(thread), ats, sizeof(struct aarch64_thread_state));
			return KERN_SUCCESS;

		case AARCH64_FLOAT_STATE:
			if (count < AARCH64_FLOAT_STATE_COUNT)
				return KERN_INVALID_ARGUMENT;
			if (((vm_offset_t) tstate) % alignof(struct aarch64_float_state))
				return KERN_INVALID_ARGUMENT;
			afs = (struct aarch64_float_state *) tstate;

			if (!validate_fpcr(afs->fpcr, old_fpr(thread, fpcr)))
				return KERN_INVALID_ARGUMENT;
			if (!validate_fpsr(afs->fpsr, old_fpr(thread, fpsr)))
				return KERN_INVALID_ARGUMENT;
			if (!validate_fpmr(afs->fpmr, old_fpr(thread, fpmr)))
				return KERN_INVALID_ARGUMENT;

			fpu_flush_state_write(thread);
			memcpy(thread->pcb->afs, tstate, sizeof(struct aarch64_float_state));
			return KERN_SUCCESS;

		default:
			return KERN_INVALID_ARGUMENT;
	}
}

/*
 * Return preferred address of user stack.
 * Always returns low address.  If stack grows up,
 * the stack grows away from this address;
 * if stack grows down, the stack grows towards this
 * address.
 */
vm_offset_t user_stack_low(vm_size_t stack_size)
{
	return (VM_MAX_USER_ADDRESS - stack_size);
}



vm_offset_t set_user_regs(
	vm_offset_t		stack_base,
	vm_offset_t		stack_size,
	const struct exec_info	*exec_info,
	vm_size_t		arg_size)
{
	struct aarch64_thread_state	*ats;
	vm_offset_t			arg_addr;

	arg_size = P2ROUND(arg_size, 16);
	arg_addr = stack_base + stack_size - arg_size;
	assert(P2ALIGNED(stack_base, 16));

	ats = USER_REGS(current_thread());
	ats->pc = exec_info->entry;
	ats->sp = (rpc_vm_offset_t) arg_addr;

	return arg_addr;
}
