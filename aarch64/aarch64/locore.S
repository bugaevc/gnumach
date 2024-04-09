#include "mach/machine/asm.h"
#include "aarch64/aarch64asm.h"
#include "aarch64/cpu_number.h"
#include "aarch64/vm_param.h"
#include <mach/kern_return.h>
#include <mach/exception.h>

/*
 *	The top (highest addresses) of each kernel stack contain:
 *
 *	struct aarch64_exception_link;
 *	struct aarch64_kernel_state;
 *
 *	where aarch64_exception_link contains a single pointer into
 *	the PCB.
 */
#define GET_PCB_STACK(reg)				 	 \
	mov	reg, sp						;\
	orr	reg, reg, #(KERNEL_STACK_SIZE - 1)		;\
	ldr	reg, [reg, #(1 - AKS_SIZE - AEL_SIZE)]

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
 *	be able to restore them back later (in thread_exception_return).
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
 *	CFI / DWARF magic to let GDB know about aarch64_exception_link
 *	& aarch64_thread_state.  This makes it possible to backtrace
 *	through an exception taken from EL0.
 */
#define DW_CFA_def_cfa_expression	0x0f

#define DW_OP_const1u			0x08
#define DW_OP_const2u			0x0a
#define DW_OP_const4u			0x0c
#define DW_OP_deref			0x06
#define DW_OP_minus			0x1c
#define DW_OP_or			0x21
#define DW_OP_reg(n)			(0x50 + (n))
#define DW_OP_breg(n)			(0x70 + (n))

#if ((KERNEL_STACK_SIZE - 1) > 0xffff) || (AKS_SIZE + AEL_SIZE - 1 > 0xff)
#error "Update the DWARF bytecode below"
#endif

#define CFI_PCB_EL0_STATE()					 \
	/*
	 *	CFA = *(
	 *		(sp | (KERNEL_STACK_SIZE - 1))
	 *		- (AKS_SIZE + AEL_SIZE - 1)
	 *	)
	 */							;\
	.cfi_escape DW_CFA_def_cfa_expression, 10,		 \
		DW_OP_breg(31), 0,				 \
		DW_OP_const2u,					 \
			(KERNEL_STACK_SIZE - 1) & 0xff,		 \
			((KERNEL_STACK_SIZE - 1) >> 8) & 0xff,	 \
		DW_OP_or,					 \
		DW_OP_const1u, AKS_SIZE + AEL_SIZE - 1,		 \
		DW_OP_minus,					 \
		DW_OP_deref					;\
	.cfi_offset 0, ATS_X					;\
	.cfi_offset 1, ATS_X + 8				;\
	.cfi_offset 2, ATS_X + 16				;\
	.cfi_offset 3, ATS_X + 24				;\
	.cfi_offset 4, ATS_X + 32				;\
	.cfi_offset 5, ATS_X + 40				;\
	.cfi_offset 6, ATS_X + 48				;\
	.cfi_offset 7, ATS_X + 56				;\
	.cfi_offset 8, ATS_X + 64				;\
	.cfi_offset 9, ATS_X + 72				;\
	.cfi_offset 10, ATS_X + 80				;\
	.cfi_offset 11, ATS_X + 88				;\
	.cfi_offset 12, ATS_X + 96				;\
	.cfi_offset 13, ATS_X + 104				;\
	.cfi_offset 14, ATS_X + 112				;\
	.cfi_offset 15, ATS_X + 120				;\
	.cfi_offset 16, ATS_X + 128				;\
	.cfi_offset 17, ATS_X + 136				;\
	.cfi_offset 18, ATS_X + 144				;\
	.cfi_offset 19, ATS_X + 152				;\
	.cfi_offset 20, ATS_X + 160				;\
	.cfi_offset 21, ATS_X + 168				;\
	.cfi_offset 22, ATS_X + 176				;\
	.cfi_offset 23, ATS_X + 184				;\
	.cfi_offset 24, ATS_X + 192				;\
	.cfi_offset 25, ATS_X + 200				;\
	.cfi_offset 26, ATS_X + 208				;\
	.cfi_offset 27, ATS_X + 216				;\
	.cfi_offset 28, ATS_X + 224				;\
	.cfi_offset 29, ATS_X + 232				;\
	.cfi_offset 30, ATS_X + 240				;\
	.cfi_offset 31, ATS_SP					;\
	.cfi_offset 32, ATS_PC					;\
	.cfi_offset 36, ATS_TPIDR_EL0


/*
 *	Spill caller-saved registers onto the stack, forming an
 *	struct aarch64_kernel_exception_state (akes).
 */

#if (AKES_X != 0) || (AKES_CPSR != AKES_PC + 8)
#error "Update the logic below"
#endif

/* 13 instructions (52 bytes) long */
#define SAVE_EL1_STATE()					 \
	stp	x0, x1, [sp, #(-AKES_SIZE)]!			;\
	.cfi_def_cfa sp, AKES_SIZE				;\
	.cfi_offset 0, AKES_X - AKES_SIZE			;\
	.cfi_offset 1, AKES_X + 8 - AKES_SIZE			;\
	stp	x2, x3, [sp, #(AKES_X + 16)]			;\
	.cfi_offset 2, AKES_X + 16 - AKES_SIZE			;\
	.cfi_offset 3, AKES_X + 24 - AKES_SIZE			;\
	stp	x4, x5, [sp, #(AKES_X + 32)]			;\
	.cfi_offset 4, AKES_X + 32 - AKES_SIZE			;\
	.cfi_offset 5, AKES_X + 40 - AKES_SIZE			;\
	stp	x6, x7, [sp, #(AKES_X + 48)]			;\
	.cfi_offset 6, AKES_X + 48 - AKES_SIZE			;\
	.cfi_offset 6, AKES_X + 56 - AKES_SIZE			;\
	stp	x8, x9, [sp, #(AKES_X + 64)]			;\
	.cfi_offset 8, AKES_X + 64 - AKES_SIZE			;\
	.cfi_offset 9, AKES_X + 72 - AKES_SIZE			;\
	stp	x10, x11, [sp, #(AKES_X + 80)]			;\
	.cfi_offset 10, AKES_X + 80 - AKES_SIZE			;\
	.cfi_offset 11, AKES_X + 80 - AKES_SIZE			;\
	stp	x12, x13, [sp, #(AKES_X + 96)]			;\
	.cfi_offset 12, AKES_X + 96 - AKES_SIZE			;\
	.cfi_offset 13, AKES_X + 104 - AKES_SIZE		;\
	stp	x14, x15, [sp, #(AKES_X + 112)]			;\
	.cfi_offset 14, AKES_X + 112 - AKES_SIZE		;\
	.cfi_offset 15, AKES_X + 120 - AKES_SIZE		;\
	stp	x16, x17, [sp, #(AKES_X + 128)]			;\
	.cfi_offset 16, AKES_X + 128 - AKES_SIZE		;\
	.cfi_offset 17, AKES_X + 136 - AKES_SIZE		;\
	stp	x18, x30, [sp, #(AKES_X + 144)]			;\
	.cfi_offset 18, AKES_X + 144 - AKES_SIZE		;\
	.cfi_offset 30, AKES_X + 152 - AKES_SIZE		;\
	mrs	x0, ELR_EL1					;\
	mrs	x1, SPSR_EL1					;\
	stp	x0, x1, [sp, #(AKES_PC)]			;\
	.cfi_offset 32, AKES_PC - AKES_SIZE			;\

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


/*
 *	Copy memory from user's untrusted address into the
 *	kernel's memory. Return 0 for success, 1 for bad
 *	address.
 */
ENTRY(copyin)
	/*
	 *	x0: user src
	 *	x1: kernel dst
	 *	x2: count in bytes
	 *
	 *	Make sure neither src nor dst overflow; also check
	 *	that end of src < VM_MAX_USER_ADDRESS.
	 */
	adds	x3, x0, x2
	b.cs	.copyin_fail
	adds	x4, x1, x2
	b.cs	.copyin_fail
	cmp	xzr, x3, lsr #(VM_AARCH64_T0SZ)
	b.ne	.copyin_fail
	/*
	 *	x3: end of src
	 *	x4: end of dst
	 *
	 *	Copy single bytes until src is 8-byte aligned.
	 *	This is a slow path, it doesn't have to be fast.
	 */
.copyin_align:
	tst	x0, #7
	b.eq	.copyin_by_8
	RECOVER(.copyin_fail)
	ldtrb	w5, [x0]		/* load a byte */
	add	x0, x0, #1
	strb	w5, [x1], #1		/* store it back */
	subs	x2, x2, #1
	b.ne	.copyin_align
.copyin_by_8:
	cmp	x2, #8
	b.lo	.copyin_rest
	RECOVER(.copyin_fail)
	ldtr	x5, [x0]		/* load 8b */
	add	x0, x0, #8
	str	x5, [x1], #8		/* store it back */
	subs	x2, x2, #8
	b.ne	.copyin_by_8
.copyin_rest:
	cbz	x2, .copyin_success
	cmp	x2, #3
	b.eq	.copyin_3
	b.lo	.copyin_1_or_2
	/* fallthrough */
.copyin_4_to_7:
	RECOVER(.copyin_fail)
	ldtr	w5, [x0]		/* load first 4b */
	RECOVER(.copyin_fail)
	ldtr	w6, [x3, #-4]		/* load last 4b */
	str	w5, [x1]		/* store first 4b */
	str	w6, [x4, #-4]		/* store last 4b */
	b	.copyin_success
.copyin_3:
	RECOVER(.copyin_fail)
	ldtrb	w5, [x0, #1]		/* load middle byte */
	strb	w5, [x1, #1]		/* store middle byte */
	/* fallthrough */
.copyin_1_or_2:
	RECOVER(.copyin_fail)
	ldtrb	w5, [x0]		/* load first byte */
	RECOVER(.copyin_fail)
	ldtrb	w6, [x3, #-1]		/* load last byte */
	strb	w5, [x1]		/* store first byte */
	strb	w6, [x4, #-1]		/* store last byte */
	/* fallthrough */
.copyin_success:
	mov	x0, #0
	ret
.copyin_fail:
	mov	x0, #1
	ret
END(copyin)

/*
 *	Copy from kernel's memory to user's untrusted address.
 *	Return 0 for success, 1 for bad address.
 */
ENTRY(copyout)
	/*
	 *	x0: kernel src
	 *	x1: user dst
	 *	x2: count in bytes
	 *
	 *	Make sure that neither src nor dst overflow;
	 *	also check that end of dst < VM_MAX_USER_ADDRESS.
	 */
	adds	x3, x0, x2
	b.cs	.copyout_fail
	adds	x4, x1, x2
	b.cs	.copyout_fail
	cmp	xzr, x4, lsr #(VM_AARCH64_T0SZ)
	b.ne	.copyout_fail
	/*
	 *	x3: end of src
	 *	x4: end of dst
	 *
	 *	Copy single bytes until dst is 8-byte aligned.
	 *	This is a slow path, it doesn't have to be fast.
	 */
.copyout_align:
	tst	x1, #7
	b.eq	.copyout_by_8
	ldrb	w5, [x0], #1		/* load a byte */
	RECOVER(.copyout_fail)
	sttrb	w5, [x1]		/* store it back */
	add	x1, x1, #1
	sub	x2, x2, #1
	b.ne	.copyout_align
.copyout_by_8:
	cmp	x2, #8
	b.lo	.copyout_rest
	ldr	x5, [x0], #8		/* load 8b */
	RECOVER(.copyout_fail)
	sttr	x5, [x1]		/* store it back */
	add	x1, x1, #8
	subs	x2, x2, #8
	b.ne	.copyout_by_8
.copyout_rest:
	cbz	x2, .copyout_success
	cmp	x2, #3
	b.eq	.copyout_3
	b.lo	.copyout_1_or_2
	/* fallthrough */
.copyout_4_to_7:
	ldr	w5, [x0]		/* load first 4b */
	ldr	w6, [x3, #-4]		/* load last 4b */
	RECOVER(.copyout_fail)
	sttr	w5, [x1]		/* store first 4b */
	RECOVER(.copyout_fail)
	sttr	w6, [x4, #-4]		/* store last 4b */
	b	.copyout_success
.copyout_3:
	ldrb	w5, [x0, #1]		/* load middle byte */
	RECOVER(.copyout_fail)
	sttrb	w5, [x1, #1]		/* store middle byte */
	/* fallthrough */
.copyout_1_or_2:
	ldrb	w5, [x0]		/* load first byte */
	ldrb	w6, [x3, #-1]		/* load last byte */
	RECOVER(.copyout_fail)
	sttrb	w5, [x1]		/* store first byte */
	RECOVER(.copyout_fail)
	sttrb	w6, [x4, #-1]		/* store last byte */
	/* fallthrough */
.copyout_success:
	mov	x0, #0
	ret
.copyout_fail:
	mov	x0, #1
	ret
END(copyout)

/*
 *	Save FP/AdvSIMD state into a struct aarch64_float_state.
 */
ENTRY(_fpu_save_state)
	stp	q0, q1, [x0], #32
	stp	q2, q3, [x0], #32
	stp	q4, q5, [x0], #32
	stp	q6, q7, [x0], #32
	stp	q8, q9, [x0], #32
	stp	q10, q11, [x0], #32
	stp	q12, q13, [x0], #32
	stp	q14, q15, [x0], #32
	stp	q16, q17, [x0], #32
	stp	q18, q19, [x0], #32
	stp	q20, q21, [x0], #32
	stp	q22, q23, [x0], #32
	stp	q24, q25, [x0], #32
	stp	q26, q27, [x0], #32
	stp	q28, q29, [x0], #32
	stp	q30, q31, [x0], #32
	mrs	x1, fpcr
	mrs	x2, fpsr
	stp	x1, x2, [x0], #32
	str	xzr, [x0]		/* FPMR ignored for now */
	ret
END(_fpu_save_state)

/*
 *	Load FP/AdvSIMD state from a struct aarch64_float_state.
 */
ENTRY(_fpu_load_state)
	ldp	q0, q1, [x0], #32
	ldp	q2, q3, [x0], #32
	ldp	q4, q5, [x0], #32
	ldp	q6, q7, [x0], #32
	ldp	q8, q9, [x0], #32
	ldp	q10, q11, [x0], #32
	ldp	q12, q13, [x0], #32
	ldp	q14, q15, [x0], #32
	ldp	q16, q17, [x0], #32
	ldp	q18, q19, [x0], #32
	ldp	q20, q21, [x0], #32
	ldp	q22, q23, [x0], #32
	ldp	q24, q25, [x0], #32
	ldp	q26, q27, [x0], #32
	ldp	q28, q29, [x0], #32
	ldp	q30, q31, [x0], #32

	ldp	x1, x2, [x0]
	msr	fpcr, x1
	msr	fpsr, x2
	/* FPMR ignored for now */
	ret
END(_fpu_load_state)

LEXT(kernel_trap_fatal_helper)
	/*
	 *	We got here after kernel_trap_sync() returned FALSE.
	 *	AKES and ESR/FAR are on the stack.  Construct an ATS
	 *	and branch to kernel_trap_fatal().
	 */
	ldp	x0, x1, [sp]			/* x0 <- esr, x1 <- far */
	ldp	x2, x3, [sp, #(16 + AKES_PC)]	/* x2 <- pc, x3 <- cpsr */

	ldp	x5, x6, [sp, #16]		/* x0, x1 */
	stp	x5, x6, [sp, #(AKES_SIZE + 16 - ATS_SIZE)]!
	ldp	x5, x6, [sp, #(ATS_SIZE - AKES_SIZE + 16)]
	stp	x5, x6, [sp, #16]		/* x2, x3 */
	ldp	x5, x6, [sp, #(ATS_SIZE - AKES_SIZE + 32)]
	stp	x5, x6, [sp, #32]		/* x4, x5 */
	ldp	x5, x6, [sp, #(ATS_SIZE - AKES_SIZE + 48)]
	stp	x5, x6, [sp, #48]		/* x6, x7 */
	ldp	x5, x6, [sp, #(ATS_SIZE - AKES_SIZE + 64)]
	stp	x5, x6, [sp, #64]		/* x8, x9 */
	ldp	x5, x6, [sp, #(ATS_SIZE - AKES_SIZE + 80)]
	stp	x5, x6, [sp, #80]		/* x10, x11 */
	ldp	x5, x6, [sp, #(ATS_SIZE - AKES_SIZE + 96)]
	stp	x5, x6, [sp, #96]		/* x12, x13 */
	ldp	x5, x6, [sp, #(ATS_SIZE - AKES_SIZE + 112)]
	stp	x5, x6, [sp, #112]		/* x14, x15 */
	ldp	x5, x6, [sp, #(ATS_SIZE - AKES_SIZE + 128)]
	stp	x5, x6, [sp, #128]		/* x16, x17 */
	ldp	x5, x6, [sp, #(ATS_SIZE - AKES_SIZE + 144)]
	stp	x5, x19, [sp, #144]		/* x18; x6 <- x30 */
	stp	x20, x21, [sp, #160]
	stp	x22, x23, [sp, #176]
	stp	x24, x25, [sp, #192]
	stp	x26, x27, [sp, #208]
	stp	x28, x29, [sp, #224]
	add	x4, sp, #ATS_SIZE
	stp	x6, x4, [sp, #240]
	stp	x2, xzr, [sp, #256]
	str	x3, [sp, #272]

	mov	x2, sp
	b	EXT(kernel_trap_fatal)
END(kernel_trap_fatal_helper)

	.section .rodata
.tbr_printf_format:
	.asciz "thread_exception_return() to %p\n"
	.text

/*
 *	Called as a function, makes the current thread
 *	return from the kernel as if from a syscall.
 *	Takes the syscall's return code as an argument.
 */
ENTRY(thread_syscall_return)
	GET_PCB_STACK(x1)
	str	x0, [x1, #(ATS_X)]
	/* b	EXT(thread_exception_return) */
	/* fallthrough */
END(thread_syscall_return)

/*
 *	Called as a function, makes the current thread
 *	return from the kernel as if from an exception.
 *
 *	Take an AST if needed, switch to the PCB stack, restore
 *	the saved register values, and return to EL0.
 */
ENTRY(thread_exception_return)
ENTRY(thread_bootstrap_return)
#ifdef __ARM_FEATURE_BTI_DEFAULT
	bti	c
#endif

	GET_PCB_STACK(x1)
	ldr	x1, [x1, #ATS_PC]
	adr	x0, .tbr_printf_format
	bl	EXT(printf)

	/*
	 *	We're about to write system registers (ELR_EL1 etc.),
	 *	and we can't allow an interrupt to clobber their values,
	 *	otherwise the 'eret' below wouldn't return to user, it
	 *	would loop back here.  So, mask interrupts for a short
	 *	while.  They'll get unmasked when we 'eret' to user code,
	 *	which sets DAIF again from SPSR_EL1.
	 *
	 */
	msr	DAIFSet, #15

	/* take an AST if needed */
	adr	x0, EXT(need_ast)
	ldr	w1, [x0]
	cbnz	w1, .take_ast

	/* switch to PCB stack */
	GET_PCB_STACK(x1)
	mov	sp, x1

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
	bl	EXT(ast_taken)		/* take the AST */
	b	EXT(thread_exception_return)
END(thread_exception_return)

LEXT(return_to_kernel)
	msr	DAIFSet, #15		/* see above */

	ldp	x0, x1, [sp, #(AKES_PC)]
	msr	ELR_EL1, x0
	msr	SPSR_EL1, x1

	ldp	x0, x1, [sp, #(AKES_X)]
	ldp	x2, x3, [sp, #(AKES_X + 16)]
	ldp	x4, x5, [sp, #(AKES_X + 32)]
	ldp	x6, x7, [sp, #(AKES_X + 48)]
	ldp	x8, x9, [sp, #(AKES_X + 64)]
	ldp	x10, x11, [sp, #(AKES_X + 80)]
	ldp	x12, x13, [sp, #(AKES_X + 96)]
	ldp	x14, x15, [sp, #(AKES_X + 112)]
	ldp	x16, x17, [sp, #(AKES_X + 128)]
	ldp	x18, x30, [sp, #(AKES_X + 144)]
	add	sp, sp, #(AKES_SIZE)
	eret
END(return_to_kernel)

/*
 *	Discard the current stack, and invoke the given continuation.
 */
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
	.cfi_startproc simple
	.cfi_signal_frame
	SAVE_EL1_STATE()
	mrs	x0, ESR_EL1
	mrs	x1, FAR_EL1
	mov	x2, sp
	stp	x0, x1, [sp, #-16]!
	.cfi_adjust_cfa_offset 16
	bl	EXT(kernel_trap_sync)
	cbz	x0, EXT(kernel_trap_fatal_helper)
	add	sp, sp, #16
	b	EXT(return_to_kernel)
	.cfi_endproc
.balign 0x80
.irq_el1_sp_el1:
	.cfi_startproc simple
	.cfi_signal_frame
	SAVE_EL1_STATE()
#ifdef MACH_KDB
	msr	DAIFClr, #8
#endif
	bl	EXT(kernel_trap_irq)
	b	EXT(return_to_kernel)
	.cfi_endproc
.balign 0x80
.fiq_el1_sp_el1:
	.cfi_startproc simple
	.cfi_signal_frame
	SAVE_EL1_STATE()
#ifdef MACH_KDB
	msr	DAIFClr, #8
#endif
	bl	EXT(kernel_trap_fiq)
	b	EXT(return_to_kernel)
	.cfi_endproc
.balign 0x80
.serror_el1_sp_el1:
	.cfi_startproc simple
	.cfi_signal_frame
	SAVE_EL1_STATE()
#ifdef MACH_KDB
	msr	DAIFClr, #8
#endif
	bl	EXT(kernel_trap_serror)
	b	EXT(return_to_kernel)
	.cfi_endproc
.balign 0x80
.sync_exc_el0_aarch64:
	.cfi_startproc
	.cfi_signal_frame
	SAVE_EL0_STATE()
	SWITCH_TO_KERNEL_STACK(x0)
	CFI_PCB_EL0_STATE()
	msr	DAIFClr, #15			/* safe to unmask interrupts now */
	bl	EXT(user_trap_sync)
	nop					/* mysterious nop */
	.cfi_endproc
.balign 0x80
.irq_el0_aarch64:
	.cfi_startproc
	.cfi_signal_frame
	SAVE_EL0_STATE()
	SWITCH_TO_KERNEL_STACK(x0)
	CFI_PCB_EL0_STATE()
#ifdef MACH_KDB
	msr	DAIFClr, #8
#endif
	bl	EXT(user_trap_irq)
	nop					/* mysterious nop */
	.cfi_endproc
.balign 0x80
.fiq_el0_aarch64:
	.cfi_startproc
	.cfi_signal_frame
	SAVE_EL0_STATE()
	SWITCH_TO_KERNEL_STACK(x0)
	CFI_PCB_EL0_STATE()
#ifdef MACH_KDB
	msr	DAIFClr, #8
#endif
	bl	EXT(user_trap_fiq)
	nop					/* mysterious nop */
	.cfi_endproc
.balign 0x80
.serror_el0_aarch64:
	.cfi_startproc
	.cfi_signal_frame
	SAVE_EL0_STATE()
	SWITCH_TO_KERNEL_STACK(x0)
	CFI_PCB_EL0_STATE()
#ifdef MACH_KDB
	msr	DAIFClr, #8
#endif
	bl	EXT(user_trap_serror)
	nop					/* mysterious nop */
	.cfi_endproc
.balign 0x80
.sync_exc_el0_aarch32:
	SWITCH_TO_KERNEL_STACK(x0)
	b	EXT(user_trap_aarch32)
.balign 0x80
.irq_el0_aarch32:
	SWITCH_TO_KERNEL_STACK(x0)
	b	EXT(user_trap_aarch32)
.balign 0x80
.fiq_el0_aarch32:
	SWITCH_TO_KERNEL_STACK(x0)
	b	EXT(user_trap_aarch32)
.balign 0x80
.serror_el0_aarch32:
	SWITCH_TO_KERNEL_STACK(x0)
	b	EXT(user_trap_aarch32)
END(exception_vector_table)

ENTRY(load_exception_vector_table)
	adr	x0, exception_vector_table
	msr	VBAR_EL1, x0
	ret
END(load_exception_vector_table)

ENTRY(handle_syscall)
	.cfi_startproc
	stp	x29, x30, [sp, #-32]!
	.cfi_def_cfa_offset 32
	.cfi_offset 29, -32
	.cfi_offset 30, -24
	mov	x29, sp
	str	x0, [sp, #16]

	mov	x12, x0
	ldr	x8, [x12, #(ATS_X + 8*8)]	/* load syscall number (in w8) */
	neg	w8, w8				/* negate it */
	adr	x9, EXT(mach_trap_count)	/* this should really be a compile-time constant... */
	ldr	x9, [x9]
	cmp	x9, x8
	b.ls	.bad_syscall
	ubfiz	x8, x8, #5, #32			/* x8 *= sizeof(mach_trap_t) */
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
	str	x0, [x12, #(ATS_X)]		/* put return code into user's x0 */
	mov	w0, #1				/* return TRUE */

.out:
	mov	sp, x29
	ldp	x29, x30, [sp], #32
	.cfi_restore x29
	.cfi_restore x30
	.cfi_def_cfa_offset 0
	ret
.load_stack_args:
	sub	x10, x10, #8
	mov	x0, x10
	add	x1, x10, #1
	tst	x0, #1
	lsl	x0, x0, #3
	lsl	x1, x1, #3
	csel	x0, x0, x1, eq
	ldr	x13, [x12, #(ATS_SP)]
	adds	x13, x13, x0
	b.cs	.bad_sp
	cmp	xzr, x13, lsr #(VM_AARCH64_T0SZ)
	b.ne	.bad_sp
0:
	sub	x13, x13, #16
	RECOVER(.bad_stack_args)
	ldtr	x1, [x13, #8]
	RECOVER(.bad_stack_args)
	ldtr	x0, [x13]
	stp	x0, x1, [sp, #-16]!
	cmp	x10, #2
	b.ls	.load_reg_args
	sub	x10, x10, #2
	b	0b
.bad_syscall:
	mov	w0, #0				/* return FALSE */
	b	.out
.bad_sp:
	mov	x0, #(EXC_BAD_ACCESS)
	mov	x1, #(KERN_INVALID_ADDRESS)
	mov	x2, x13
	/* fallthrough */
.bad_stack_args:
	/*
	 *	We were loading arguments from user's stack,
	 *	but faulted.  Restore our SP, and treat it
	 *	as a fault from EL0.  x0, x1, x2 already hold
	 *	appropriate values for the exception() call.
	 */
	mov	sp, x29
	ldp	x29, x30, [sp], #32
	b	EXT(exception)
	.cfi_endproc
END(handle_syscall)

	.pushsection .rodata, 2
END(recover_table)
	.global EXT(recover_table_end)
LEXT(recover_table_end)
	.popsection

	.section .note.GNU-stack,"",%progbits
