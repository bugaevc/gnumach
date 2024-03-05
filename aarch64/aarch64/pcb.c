#include "pcb.h"
#include "aarch64/vm_param.h"
#include "aarch64/pmap.h"
#include "aarch64/fpu.h"
#include <vm/vm_map.h>
#include <kern/slab.h>
#include <kern/counters.h>
#include <string.h>

#define SPSR_M_EL(spsr)		((spsr) & 0xf)
#define SPSR_M_EL_EL0		0x0		/* EL0 */
#define SPSR_M_EL_EL1_SP_EL0	0x4		/* EL1 with SP_EL0 */
#define SPSR_M_EL_EL1_SP_EL1	0x5		/* EL1 with SP_EL1 */

#define SPSR_M_AARCH(spsr)	((spsr) & 0x10)
#define SPSR_M_AARCH_AARCH64	0x00		/* AArch64 */
#define SPSR_M_AARCH_AARCH32	0x10		/* AArch32 */

#define SPSR_PAN		0x400000	/* privileged access never */
#define SPSR_UAO		0x800000	/* user access override */


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

kern_return_t thread_setstatus(
	thread_t	thread,
	int		flavor,
	thread_state_t	tstate,
	unsigned int	count)
{
	struct aarch64_thread_state *ats;

	switch (flavor) {
		case AARCH64_THREAD_STATE:
			if (count < AARCH64_THREAD_STATE_COUNT)
				return KERN_INVALID_ARGUMENT;
			if (((vm_offset_t) tstate) % alignof(struct aarch64_thread_state))
				return KERN_INVALID_ARGUMENT;
			ats = (struct aarch64_thread_state *) tstate;

			/*
			 *	Make sure the PSR indicates a valid state,
			 *	specifically EL0 AArch64.
			 *
			 *	Note that both SPSR_M_EL_EL0 and
			 *	SPSR_M_AARCH_AARCH64 have a zero bit pattern,
			 *	so just initializing ats->cpsr to 0 should
			 *	pass the checks successfully.
			 */
			if (SPSR_M_EL(ats->cpsr) != SPSR_M_EL_EL0)
				return KERN_INVALID_ARGUMENT;
			if (SPSR_M_AARCH(ats->cpsr) != SPSR_M_AARCH_AARCH64)
				return KERN_INVALID_ARGUMENT;
			if (ats->cpsr & SPSR_UAO)
				return KERN_INVALID_ARGUMENT;

			/* TODO: DAIF; should probably mask in the right value
			   instead of requiring the caller to supply it */
			ats->cpsr |= SPSR_PAN;
			/* TODO: audit all the other bits */

			memcpy(USER_REGS(thread), ats, sizeof(struct aarch64_thread_state));
			return KERN_SUCCESS;

		case AARCH64_FLOAT_STATE:
			if (count < AARCH64_FLOAT_STATE_COUNT)
				return KERN_INVALID_ARGUMENT;
			if (((vm_offset_t) tstate) % alignof(struct aarch64_float_state))
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
