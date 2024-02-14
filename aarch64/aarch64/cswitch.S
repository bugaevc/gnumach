#include <mach/machine/asm.h>
#include "aarch64/aarch64asm.h"

ENTRY(load_context)
	ldr	x1, [x0, #TH_KERNEL_STACK]
	mov	x2, #KSS_REGS
	add	x1, x1, x2
	ldp	x19, x20, [x1], #16
	ldp	x21, x22, [x1], #16
	ldp	x23, x24, [x1], #16
	ldp	x25, x26, [x1], #16
	ldp	x27, x28, [x1], #16
	ldp	x29, x30, [x1]
	/* No old thread.  */
	mov	x0, #0
	ret
END(load_context)

ENTRY(Thread_continue)
	mov	x29, 0
	br	x19
END(Thread_continue)
