#include "pcb.h"
#include "aarch64/vm_param.h"
#include "aarch64/pmap.h"
#include <vm/vm_map.h>
#include <kern/counters.h>

vm_offset_t	kernel_stack[NCPUS];

void pcb_module_init(void)
{
}

void stack_attach(
	thread_t	thread,
	vm_offset_t	stack,
	void		(*continuation)(thread_t))
{
	counter(if (++c_stacks_current > c_stacks_max)
		c_stacks_max = c_stacks_current);

	thread->kernel_stack = stack;
	STACK_AKS_REG(stack, 30) = (long) Thread_continue;
	STACK_AKS_REG(stack, 19) = (long) continuation;
	STACK_AKS_REG(stack, 29) = (long) 0;
	STACK_AKS(stack)->k_sp = (long) stack;

	STACK_AEL(stack)->saved_state = (pcb_t) USER_REGS(thread);
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

	return Switch_context(old, continuation, new);
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
