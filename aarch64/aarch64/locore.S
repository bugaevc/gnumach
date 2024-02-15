#include "mach/machine/asm.h"
#include "aarch64/aarch64asm.h"
#include "aarch64/cpu_number.h"

#define SWITCH_TO_PCB_STACK(scratch)				 \
	mov	scratch, sp					;\
	orr	scratch, scratch, #(KERNEL_STACK_SIZE - 1)	;\
	ldr	scratch, [scratch, #(AKS_SIZE + 7)]		;\
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
	stp	x2, x3, [sp, #(PCB_ESR)]


/*
 *	Called as a function, makes the current thread
 *	return from the kernel as if from an exception.
 */
ENTRY(thread_exception_return)
ENTRY(thread_bootstrap_return)
	SWITCH_TO_PCB_STACK(x1)
	b	return_from_trap
END(thread_exception_return)

/*
 *	Called as a function, makes the current thread
 *	return from the kernel as if from a syscall.
 *	Takes the syscall's return code as an argument.
 */
ENTRY(thread_syscall_return)
	SWITCH_TO_PCB_STACK(x1)
	str	x0, [sp, #(ATS_X)]
	b	return_from_trap
END(thread_syscall_return)

.type return_from_trap, @function
LEXT(return_from_trap)
	/* running on PCB stack */
	// CPU_NUMBER(x0)
	// ldrb	w1, CX(EXT(need_ast), x0)
	adr	x0, need_ast
	tbnz	x1, 0, .take_ast

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

.take_ast:
	SWITCH_TO_KERNEL_STACK(x1)
	bl	EXT(ast_taken)		/* take the AST */
	SWITCH_TO_PCB_STACK(x1)
	b	return_from_trap	/* try again */
END(return_from_trap)

ENTRY(call_continuation)
	SWITCH_TO_PCB_STACK(x1)
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
	b	.
.balign 0x80
.irq_el1_sp_el1:
	b	.
.balign 0x80
.fiq_el1_sp_el1:
	b	.
.balign 0x80
.serror_el1_sp_el1:
	b	.
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
