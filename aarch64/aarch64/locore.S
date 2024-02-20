#include "mach/machine/asm.h"
#include "aarch64/aarch64asm.h"
#include "aarch64/cpu_number.h"

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
	mrs	x2, ESR_EL1					;\
	mrs	x3, FAR_EL1					;\
	stp	x2, x3, [sp, #(ATS_SIZE)]

/* 22 instructions (88 bytes) long */
#define SAVE_EL1_STATE()					 \
	/*
	 *	Spill caller-saved registers onto the current stack,
	 *	which might be the PCB stack or the kernel stack.
	 */							;\
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
	/*
	 *	If we were on the PCB stack, switch to the kernel stack.
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
	/* FIXME: SPSR, ELR */


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
	SWITCH_TO_PCB_STACK(x1)
	/* b	return_to_user */
	/* fallthrough */
END(thread_exception_return)

.type EXT(return_to_user), @function
LEXT(return_to_user)
	/* running on PCB stack */
	// CPU_NUMBER(x0)
	// ldrb	w1, CX(EXT(need_ast), x0)
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
	br	x0			/* goto continuation */
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
	ldr	x12, [sp, #16]
	str	x0, [x12, #(ATS_X)]		/* put return code into user's x0 */
	mov	w0, #1				/* return TRUE */

.out:
	ldp	x29, x30, [sp], #32
	ret
.bad_syscall:
	mov	w0, #0				/* return FALSE */
	b	.out
.load_stack_args:
	/* FIXME */
	b	.
END(handle_syscall)
