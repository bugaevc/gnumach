#include <mach/machine/asm.h>
#include "aarch64/aarch64asm.h"

ENTRY(load_context)
	ldr	x1, [x0, #TH_KERNEL_STACK]
	adr	x0, kernel_stack
	str	x1, [x0]
	mov	x2, #KSS_REGS
	add	x1, x1, x2
	ldp	x19, x20, [x1], #16
	ldp	x21, x22, [x1], #16
	ldp	x23, x24, [x1], #16
	ldp	x25, x26, [x1], #16
	ldp	x27, x28, [x1], #16
	ldp	x29, x30, [x1]
	mov	x0, #0			/* no old thread */
	ret
END(load_context)

ENTRY(Switch_context)
	adr	x4, EXT(kernel_stack)
	ldr	x6, [x4]
	mov	x5, #KSS_REGS
	add	x5, x6, x5

	stp	x19, x20, [x5], #16
	stp	x21, x22, [x5], #16
	stp	x23, x24, [x5], #16
	stp	x25, x26, [x5], #16
	stp	x27, x28, [x5], #16
	stp	x29, x30, [x5], #16
	mov	x3, sp
	str	x3, [x5]

	str	x6, [x0, #TH_KERNEL_STACK]
	str	x1, [x0, #TH_SWAP_FUNC]
	ldr	x7, [x2, #TH_KERNEL_STACK]
	mov	x5, #(KERNEL_STACK_SIZE-AKS_SIZE-AEL_SIZE)
	add	x8, x7, x5
	str	x8, [x4]		/* update kernel_stack */

	/* update current_thread() */
	adr	x4, EXT(percpu_array)
	str	x2, [x4, #PERCPU_ACTIVE_THREAD]

	mov	x5, #KSS_REGS
	add	x5, x7, x5

	ldp	x19, x20, [x5], #16
	ldp	x21, x22, [x5], #16
	ldp	x23, x24, [x5], #16
	ldp	x25, x26, [x5], #16
	ldp	x27, x28, [x5], #16
	ldp	x29, x30, [x5], #16
	ldr	x2, [x5]
	mov	sp, x2

	ret
END(Switch_context)

ENTRY(Thread_continue)
	mov	x29, #0
	br	x19
END(Thread_continue)
