#include "pcb.h"
#include "aarch64/vm_param.h"

void pcb_module_init(void)
{
}

/*
 * Return preferred address of user stack.
 * Always returns low address.  If stack grows up,
 * the stack grows away from this address;
 * if stack grows down, the stack grows towards this
 * address.
 */
vm_offset_t
user_stack_low(vm_size_t stack_size)
{
	return (VM_MAX_USER_ADDRESS - stack_size);
}
