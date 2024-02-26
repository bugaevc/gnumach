#include "pcb.h"
#include "aarch64/vm_param.h"
#include "aarch64/pmap.h"
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
			/* TODO */
			return KERN_MEMORY_FAILURE;
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
