#include <mach/machine/asm.h>
#include "aarch64/aarch64asm.h"
#include "aarch64/vm_param.h"

ENTRY(load_context)
	ldr	x1, [x0, #TH_KERNEL_STACK]	/* load kernel stack bottom */
	mov	x2, #(KERNEL_STACK_SIZE-AKS_SIZE-AEL_SIZE)
	add	x2, x1, x2			/* find kernel stack top */
	adr	x0, EXT(kernel_stack)
	str	x2, [x0]			/* store kernel stack top */
	mrs	x2, TPIDR_EL1
	str	x1, [x2, #PERCPU_ACTIVE_STACK]	/* store stack bottom */
	mov	x2, #KSS_REGS			/* load registers... */
	add	x1, x1, x2			/* ...relative to stack bottom */
	ldp	x19, x20, [x1], #16
	ldp	x21, x22, [x1], #16
	ldp	x23, x24, [x1], #16
	ldp	x25, x26, [x1], #16
	ldp	x27, x28, [x1], #16
	ldp	x29, x30, [x1], #16
	ldr	x0, [x1]
	mov	sp, x0
	mov	x0, #0				/* no old thread */
	ret
END(load_context)

ENTRY(Switch_context)
	mrs	x4, TPIDR_EL1
	ldr	x6, [x4, #PERCPU_ACTIVE_STACK]	/* load stack bottom */
	mov	x5, #KSS_REGS
	add	x5, x6, x5

	stp	x19, x20, [x5], #16		/* save registers */
	stp	x21, x22, [x5], #16
	stp	x23, x24, [x5], #16
	stp	x25, x26, [x5], #16
	stp	x27, x28, [x5], #16
	stp	x29, x30, [x5], #16
	mov	x3, sp
	str	x3, [x5]

	str	x6, [x0, #TH_KERNEL_STACK]	/* save stack bottom */
	str	x1, [x0, #TH_SWAP_FUNC]		/* save continuation */
	ldr	x7, [x2, #TH_KERNEL_STACK]	/* load new stack bottom */
	mov	x5, #(KERNEL_STACK_SIZE-AKS_SIZE-AEL_SIZE)
	add	x8, x7, x5			/* find new stack top */

	str	x2, [x4, #PERCPU_ACTIVE_THREAD]	/* update current_thread() */
	str	x7, [x4, #PERCPU_ACTIVE_STACK]	/* store stack bottom */
	adr	x4, EXT(kernel_stack)
	str	x8, [x4]			/* store stack top */

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
	mov	x30, #0
#ifdef __ARM_FEATURE_BTI_DEFAULT
	mov	x16, x19
	br	x16
#else
	br	x19
#endif
END(Thread_continue)

	.section .note.GNU-stack,"",%progbits
