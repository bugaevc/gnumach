#include <mach/machine/asm.h>
#include "aarch64/ipl.h"

/* This should be per CPU... */
.bss
.type EXT(curr_ipl), @object
LEXT(curr_ipl)
	.space 1
END(curr_ipl)

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
	msr	DAIFSet, #2
	adr	x1, EXT(curr_ipl)
	ldrb	w0, [x1]
	strb	wzr, [x1]
	ret
END(spl0)

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
	msr	DAIFClr, #2
	adr	x1, EXT(curr_ipl)
	ldrb	w0, [x1]
	mov	w2, #SPL7
	strb	w2, [x1]
	ret
END(spl7)

ENTRY(splx)
	cmp	x0, #0
	beq	EXT(spl0)
	b	EXT(spl7)
END(splx)

/*
 *	Remember to run sofclock at next spl0().
 */
ENTRY(setsoftclock)
	adr	x0, EXT(softclkpending)
	mov	w1, #1
	strb	w1, [x0]
	ret
END(setsoftclock)
