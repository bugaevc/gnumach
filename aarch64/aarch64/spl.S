#include <mach/machine/asm.h>
#include "aarch64/aarch64asm.h"
#include "aarch64/ipl.h"

.bss
.type EXT(softclkpending), @object
LEXT(softclkpending)
	.space 1
END(softclkpending)


.text
/*
 *	Enable interrupts.
 */
ENTRY(spl0)
	/* But first, check if we should run softclock().  */
	adr	x0, EXT(softclkpending)
	ldrb	w1, [x0]
	tbz	x1, 0, .do_enable
	strb	wzr, [x0]
	stp	x29, x30, [sp, -32]!
	mov	x29, sp
	bl	EXT(spl1)
	bl	EXT(softclock)
	ldp	x29, x30, [sp], 32
.do_enable:
	adr	x1, EXT(percpu_array)
	ldr	w0, [x1, #PERCPU_CURR_IPL]
	cbz	w0, 1f

	str	wzr, [x1, #PERCPU_CURR_IPL]
	msr	DAIFClr, #3
	isb

1:
	ret
END(spl0)

/*
 *	Enable or disable interrupts, depending on the argument.
 */
ENTRY(splx)
	cbz	x0, EXT(spl0)
	/* b	EXT(spl7) */
	/* fallthrough */
END(splx)

/*
 *	Disable interrupts.
 */
ENTRY(splsoftclock)
ENTRY(spl1)
ENTRY(spl2)
ENTRY(spl3)
ENTRY(splnet)
ENTRY(splhdw)
ENTRY(spl4)
ENTRY(splbio)
ENTRY(spldcm)
ENTRY(spl5)
ENTRY(spltty)
ENTRY(splimp)
ENTRY(splvm)
ENTRY(spl6)
ENTRY(splclock)
ENTRY(splsched)
ENTRY(splhigh)
ENTRY(splhi)
ENTRY(spl7)
	adr	x1, EXT(percpu_array)
	ldr	w0, [x1, #PERCPU_CURR_IPL]
	cmp	w0, #SPL7
	b.eq	1f

	msr	DAIFSet, #3
	mov	w2, #SPL7
	str	w2, [x1, #PERCPU_CURR_IPL]
1:
	ret
END(spl7)

/*
 *	Remember to run sofclock at next spl0().
 */
ENTRY(setsoftclock)
	adr	x0, EXT(softclkpending)
	mov	w1, #1
	strb	w1, [x0]
	ret
END(setsoftclock)

ENTRY(spl7_irq)
	adr	x1, EXT(percpu_array)
	ldr	w0, [x1, #PERCPU_CURR_IPL]
	mov	w2, #SPL7
	str	w2, [x1, #PERCPU_CURR_IPL]
	ret
END(spl7_irq)

/*
 *	Called at SPL7. We're about to return from
 *	an IRQ to code that runs at SPL0.
 */
ENTRY(spl0_irq)
	adr	x2, EXT(softclkpending)
	ldrb	w1, [x2]
	tbz	w1, 0, 1f
	strb	wzr, [x2]
	stp	x29, x30, [sp, -32]!
	mov	x29, sp
	bl	EXT(softclock)
	ldp	x29, x30, [sp], 32
1:
	adr	x1, EXT(percpu_array)
	str	wzr, [x1, #PERCPU_CURR_IPL]
	ret
END(spl0_irq)

	.section .note.GNU-stack,"",%progbits
