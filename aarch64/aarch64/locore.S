#include "mach/machine/asm.h"
#include "aarch64/aarch64asm.h"
#include "aarch64/cpu_number.h"

/*
 *	The top (highest addresses) of each kernel stack contain:
 *
 *	struct aarch64_exception_link;
 *	struct aarch64_kernel_state;
 *
 *	where aarch64_exception_link contains a single pointer into
 *	the PCB.  Point our stack there.
 */
#define SWITCH_TO_PCB_STACK(scratch)				 \
	mov	scratch, sp					;\
	orr	scratch, scratch, #(KERNEL_STACK_SIZE - 1)	;\
	ldr	scratch, [scratch, #(1 - AKS_SIZE - AEL_SIZE)]	;\
	mov	sp, scratch

/* 3 instructions (12 bytes) long */
#define SWITCH_TO_KERNEL_STACK(scratch)				 \
	adr	scratch, EXT(kernel_stack)			;\
	ldr	scratch, [scratch]				;\
	mov	sp, scratch

/*
 *	We just caught an exception or an interrupt from EL0; this
 *	saved pc -> ELR_EL1, replacing it with the handler's address,
 *	and sp -> SP_EL0, replacing it with the previous EL1 stack
 *	pointer (SP_EL1).  The other general-purpose registers are as
 *	they have been in EL0.  We want to save them into memory, to
 *	be able to restore them back later (in return_to_user).
 *
 *	The way we achieve this is we leave SP_EL1 pointing right to
 *	"struct aarch64_thread_state ats" member of the PCB; this is
 *	referred to as running on "the PCB stack", even though you
 *	cannot use PCB as a stack otherwise, e.g. you cannot call C
 *	code while on the PCB stack.
 */

/* 25 instructions (100 bytes) long */
#define SAVE_EL0_STATE()					 \
	stp	x0, x1, [sp, #(ATS_X)]				;\
	stp	x2, x3, [sp, #(ATS_X + 16)]			;\
	stp	x4, x5, [sp, #(ATS_X + 32)]			;\
	stp	x6, x7, [sp, #(ATS_X + 48)]			;\
	stp	x8, x9, [sp, #(ATS_X + 64)]			;\
	stp	x10, x11, [sp, #(ATS_X + 80)]			;\
	stp	x12, x13, [sp, #(ATS_X + 96)]			;\
	stp	x14, x15, [sp, #(ATS_X + 112)]			;\
	stp	x16, x17, [sp, #(ATS_X + 128)]			;\
	stp	x18, x19, [sp, #(ATS_X + 144)]			;\
	stp	x20, x21, [sp, #(ATS_X + 160)]			;\
	stp	x22, x23, [sp, #(ATS_X + 176)]			;\
	stp	x24, x25, [sp, #(ATS_X + 192)]			;\
	stp	x26, x27, [sp, #(ATS_X + 208)]			;\
	stp	x28, x29, [sp, #(ATS_X + 224)]			;\
	str	x30, [sp, #(ATS_X + 240)]			;\
								;\
	mrs	x2, SP_EL0					;\
	mrs	x3, ELR_EL1					;\
	stp	x2, x3, [sp, #(ATS_SP)]				;\
	mrs	x2, TPIDR_EL0					;\
	mrs	x3, SPSR_EL1					;\
	stp	x2, x3, [sp, #(ATS_TPIDR_EL0)]			;\
	/*
	 *	ESR/FAR are not a part of aarch64_thread_state,
	 *	but they are stored in the PCB immediately
	 *	following the aarch64_thread_state.
	 */							;\
	mrs	x2, ESR_EL1					;\
	mrs	x3, FAR_EL1					;\
	stp	x2, x3, [sp, #(ATS_SIZE)]


/*
 *	Spill caller-saved registers onto the current stack,
 *	which might be the PCB stack or the kernel stack.
 */

/* 25 instructions (100 bytes) long */
#define SAVE_EL1_STATE()					 \
	stp	x0, x1, [sp, #-16]!				;\
	stp	x2, x3, [sp, #-16]!				;\
	stp	x4, x5, [sp, #-16]!				;\
	stp	x6, x7, [sp, #-16]!				;\
	stp	x8, x9, [sp, #-16]!				;\
	stp	x10, x11, [sp, #-16]!				;\
	stp	x12, x13, [sp, #-16]!				;\
	stp	x14, x15, [sp, #-16]!				;\
	stp	x16, x17, [sp, #-16]!				;\
	stp	x18, x30, [sp, #-16]!				;\
	mrs	x0, ELR_EL1					;\
	mrs	x1, SPSR_EL1					;\
	stp	x0, x1, [sp, #-16]!				;\
	/*
	 *	If we were on the PCB stack,
	 *	switch to the kernel stack.
	 */							;\
	adr	x0, EXT(percpu_array)				;\
	ldr	x0, [x0, #PERCPU_ACTIVE_STACK]			;\
	adr	x1, EXT(kernel_stack)				;\
	ldr	x1, [x1]					;\
	tst	x1, x1						;\
	b.eq	0f						;\
	mov	x2, sp						;\
	cmp	x2, x0						;\
	ccmp	x2, x1, 2, cs					;\
	b.ls	0f						;\
	mov	sp, x1						;\
0:								;\
	stp	x2, x2, [sp, #-16]!				;\


/*
 *	Fault recovery.
 */
#define RECOVER(handler)					 \
	.pushsection .rodata, 2					;\
	.xword	9f - EXT(recover_table)				;\
	.xword	handler - EXT(recover_table)			;\
	.popsection						;\
9:

	.pushsection .rodata, 2
	.global EXT(recover_table)
LEXT(recover_table)
	.popsection


#ifdef notyet
/* TODO need a way to feature-probe these */
#define INHIBIT_PAN()						 \
	.word	0xd500409f		/* msr PAN, #0 */

#define REENABLE_PAN()						 \
	.word	0xd500419f		/* msr PAN, #1 */

#else
#define INHIBIT_PAN()
#define REENABLE_PAN()
#endif

/*
 *	Copy memory from user's untrusted address into the
 *	kernel's memory. Return 0 for success, 1 for bad
 *	address.
 *
 *	TODO optimize this
 */
ENTRY(copyin)
	cbz	x2, 1f
	INHIBIT_PAN()
	mov	x3, #0
0:
	RECOVER(3f)
	ldrb	w4, [x0, x3]
	strb	w4, [x1, x3]
	add	x3, x3, #1
	cmp	x2, x3
	b.ne	0b
1:
	mov	x0, #0
2:
	REENABLE_PAN()
	ret
3:
	mov	x0, #1
	b	2b
END(copyin)

ENTRY(copyout)
	cbz	x2, 1f
	INHIBIT_PAN()
	mov	x3, #0
0:
	ldrb	w4, [x0, x3]
	RECOVER(3f)
	strb	w4, [x1, x3]
	add	x3, x3, #1
	cmp	x2, x3
	b.ne	0b
1:
	mov	x0, #0
2:
	REENABLE_PAN()
	ret
3:
	mov	x0, #1
	b	2b
END(copyout)

/*
 *	Called as a function, makes the current thread
 *	return from the kernel as if from a syscall.
 *	Takes the syscall's return code as an argument.
 */
ENTRY(thread_syscall_return)
	SWITCH_TO_PCB_STACK(x1)
	str	x0, [sp, #(ATS_X)]
	b	return_to_user
END(thread_syscall_return)

/*
 *	Called as a function, makes the current thread
 *	return from the kernel as if from an exception.
 */
ENTRY(thread_exception_return)
ENTRY(thread_bootstrap_return)
#ifdef __ARM_FEATURE_BTI_DEFAULT
	bti	c
#endif
	SWITCH_TO_PCB_STACK(x1)
	/* b	return_to_user */
	/* fallthrough */
END(thread_exception_return)

/*
 *	We're running on the PCB stack, meaning our SP points to
 *	"struct aarch64_thread_state ats" member of the PCB; that's
 *	also how we want to leave it, so it will be restored on a
 *	subsequent trap from user.
 *
 *	Take an AST if needed, restore the saved register values, and
 *	return to EL0.
 */
.type EXT(return_to_user), @function
LEXT(return_to_user)
	adr	x0, EXT(need_ast)
	ldr	w1, [x0]
	cbnz	w1, .take_ast1

	/* restore registers */
	ldp	x0, x1, [sp, #(ATS_SP)]
	msr	SP_EL0, x0
	msr	ELR_EL1, x1
	ldp	x0, x1, [sp, #(ATS_TPIDR_EL0)]
	msr	TPIDR_EL0, x0
	msr	SPSR_EL1, x1

	ldp	x0, x1, [sp, #(ATS_X)]
	ldp	x2, x3, [sp, #(ATS_X + 16)]
	ldp	x4, x5, [sp, #(ATS_X + 32)]
	ldp	x6, x7, [sp, #(ATS_X + 48)]
	ldp	x8, x9, [sp, #(ATS_X + 64)]
	ldp	x10, x11, [sp, #(ATS_X + 80)]
	ldp	x12, x13, [sp, #(ATS_X + 96)]
	ldp	x14, x15, [sp, #(ATS_X + 112)]
	ldp	x16, x17, [sp, #(ATS_X + 128)]
	ldp	x18, x19, [sp, #(ATS_X + 144)]
	ldp	x20, x21, [sp, #(ATS_X + 160)]
	ldp	x22, x23, [sp, #(ATS_X + 176)]
	ldp	x24, x25, [sp, #(ATS_X + 192)]
	ldp	x26, x27, [sp, #(ATS_X + 208)]
	ldp	x28, x29, [sp, #(ATS_X + 224)]
	ldr	x30, [sp, #(ATS_X + 240)]
	eret

.take_ast1:
	SWITCH_TO_KERNEL_STACK(x1)
	bl	EXT(ast_taken)		/* take the AST */
	SWITCH_TO_PCB_STACK(x1)
	b	EXT(return_to_user)	/* try again */
END(return_to_user)

LEXT(return_to_kernel)
	adr	x0, EXT(need_ast)
	ldr	w1, [x0]
	tbnz	x1, 0, .take_ast2

	ldp	x0, x1, [sp]		/* restore stack */
	mov	sp, x0

	ldp	x0, x1, [sp], #16
	msr	ELR_EL1, x0
	msr	SPSR_EL1, x1

	ldp	x18, x30, [sp], #16	/* restore registers */
	ldp	x16, x17, [sp], #16
	ldp	x14, x15, [sp], #16
	ldp	x12, x13, [sp], #16
	ldp	x10, x11, [sp], #16
	ldp	x8, x9, [sp], #16
	ldp	x6, x7, [sp], #16
	ldp	x4, x5, [sp], #16
	ldp	x2, x3, [sp], #16
	ldp	x0, x1, [sp], #16
	eret

.take_ast2:
	bl	EXT(ast_taken)		/* take the AST */
	b	EXT(return_to_kernel)	/* try again */
END(return_to_kernel)

ENTRY(call_continuation)
	mov	x1, sp
	orr	x1, x1, #(KERNEL_STACK_SIZE - 1)
	sub	x1, x1, #(AKS_SIZE + AEL_SIZE - 1)
	mov	sp, x1			/* point stack to the top */
	mov	x29, #0			/* dummy frame */
	mov	x30, #0			/* dummy return */
#ifdef __ARM_FEATURE_BTI_DEFAULT
	mov	x16, x0
	br	x16			/* goto continuation */
#else
	br	x0			/* goto continuation */
#endif
END(call_continuation)

	.balign 2048
ENTRY(exception_vector_table)
.sync_exc_el1_sp_el0:
	b	.
.balign 0x80
.irq_el1_sp_el0:
	b	.
.balign 0x80
.fiq_el1_sp_el0:
	b	.
.balign 0x80
.serror_el1_sp_el0:
	b	.
.balign 0x80
.sync_exc_el1_sp_el1:
	SAVE_EL1_STATE()
	mrs	x0, ESR_EL1
	mrs	x1, FAR_EL1
	/* x2 contains &aarch64_kernel_exception_state */
	bl	EXT(trap_sync_exc_el1)
	b	EXT(return_to_kernel)
.balign 0x80
.irq_el1_sp_el1:
	SAVE_EL1_STATE()
	bl	EXT(trap_irq_el1)
	b	EXT(return_to_kernel)
.balign 0x80
.fiq_el1_sp_el1:
	SAVE_EL1_STATE()
	bl	EXT(trap_fiq_el1)
	b	EXT(return_to_kernel)
.balign 0x80
.serror_el1_sp_el1:
	SAVE_EL1_STATE()
	bl	EXT(trap_serror_el1)
	b	EXT(return_to_kernel)
.balign 0x80
.sync_exc_el0_aarch64:
	SAVE_EL0_STATE()
	SWITCH_TO_KERNEL_STACK(x0)
	b	EXT(trap_sync_exc_el0)
.balign 0x80
.irq_el0_aarch64:
	SAVE_EL0_STATE()
	SWITCH_TO_KERNEL_STACK(x0)
	b	EXT(trap_irq_el0)
.balign 0x80
.fiq_el0_aarch64:
	SAVE_EL0_STATE()
	SWITCH_TO_KERNEL_STACK(x0)
	b	EXT(trap_fiq_el0)
.balign 0x80
.serror_el0_aarch64:
	SAVE_EL0_STATE()
	SWITCH_TO_KERNEL_STACK(x0)
	b	EXT(trap_serror_el0)
.balign 0x80
.sync_exc_el0_aarch32:
	SWITCH_TO_KERNEL_STACK(x0)
	b	EXT(trap_aarch32)
.balign 0x80
.irq_el0_aarch32:
	SWITCH_TO_KERNEL_STACK(x0)
	b	EXT(trap_aarch32)
.balign 0x80
.fiq_el0_aarch32:
	SWITCH_TO_KERNEL_STACK(x0)
	b	EXT(trap_aarch32)
.balign 0x80
.serror_el0_aarch32:
	SWITCH_TO_KERNEL_STACK(x0)
	b	EXT(trap_aarch32)
END(exception_vector_table)

ENTRY(load_exception_vector_table)
	adr	x0, exception_vector_table
	msr	VBAR_EL1, x0
	ret
END(load_exception_vector_table)

ENTRY(handle_syscall)
	stp	x29, x30, [sp, #-32]!
	mov	x29, sp
	str	x0, [sp, #16]

	mov	x12, x0
	ldr	x8, [x12, #(ATS_X + 8*8)]	/* load syscall number (in w8) */
	neg	w8, w8				/* negate it */
	adr	x9, EXT(mach_trap_count)	/* this should really be a compile-time constant... */
	ldr	x9, [x9]
	cmp	x9, x8
	b.ls	.bad_syscall
	ubfiz	x8, x8, #5, #32			/* $x8 *= sizeof(mach_trap_t) */
	adr	x9, EXT(mach_trap_table)
	add	x9, x9, x8
	ldp	x10, x11, [x9]
	cmp	x10, #8
	b.hi	.load_stack_args
.load_reg_args:
	ldp	x0, x1, [x12, #(ATS_X)]
	ldp	x2, x3, [x12, #(ATS_X + 16)]
	ldp	x4, x5, [x12, #(ATS_X + 32)]
	ldp	x6, x7, [x12, #(ATS_X + 48)]

	blr	x11

	ldr	x12, [x29, #16]
	ldrb	w1, [x12, #(ATS_SKIP_SYSCALL_RV)]
	tbnz	w1, 0, 1f
	str	x0, [x12, #(ATS_X)]		/* put return code into user's x0 */
	mov	w0, #1				/* return TRUE */

.out:
	mov	sp, x29
	ldp	x29, x30, [sp], #32
	ret
1:
	mov	w1, #0
	strb	w1, [x12, #(ATS_SKIP_SYSCALL_RV)]
	mov	w0, #1
	b	.out
.load_stack_args:
	sub	x10, x10, #8
	mov	x0, x10
	add	x1, x10, #1
	tst	x0, #1
	lsl	x0, x0, #3
	lsl	x1, x1, #3
	csel	x0, x0, x1, eq
	ldr	x13, [x12, #(ATS_SP)]
	add	x13, x13, x0
0:
	RECOVER(.bad_syscall)
	ldp	x0, x1, [x13, #-16]!
	stp	x0, x1, [sp, #-16]!
	cmp	x10, #2
	b.ls	.load_reg_args
	sub	x10, x10, #2
	b	0b
.bad_syscall:
	mov	w0, #0				/* return FALSE */
	b	.out
END(handle_syscall)

	.pushsection .rodata, 2
END(recover_table)
	.global EXT(recover_table_end)
LEXT(recover_table_end)
	.popsection
