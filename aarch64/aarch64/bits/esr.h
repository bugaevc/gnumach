#ifndef _AARCH64_BITS_ESR_
#define _AARCH64_BITS_ESR_

/* Extract exception class from ESR value.  */
#define ESR_EC(esr)		(((esr) >> 26) & 0x3f)
/* Exception classes.  */
#define ESR_EC_UNK		0x00		/* unknown reason */
#define ESR_EC_WF		0x01		/* WFI/WFE */
#define ESR_EC_FP_ACCESS	0x07		/* FP/AdvSIMD access when disabled */
#define ESR_EC_BTI		0x0d		/* BTI failure */
#define ESR_EC_IL		0x0e		/* illegal execution state */
#define ESR_EC_SVC		0x15		/* SVC (syscall) */
#define ESR_EC_HVC		0x16		/* HVC */
#define ESR_EC_SMC		0x17		/* SMC */
#define ESR_EC_MRS		0x18		/* MRS or MRS (or cache?) */
#define ESR_EC_PAC		0x1c		/* PAC failure */
#define ESR_EC_IABT_LOWER_EL	0x20		/* instruction abort from lower EL */
#define ESR_EC_IABT_SAME_EL	0x21		/* instruction abort from the same EL */
#define ESR_EC_AL_PC		0x22		/* misaligned PC */
#define ESR_EC_DABT_LOWER_EL	0x24		/* data abort from lower EL */
#define ESR_EC_DABT_SAME_EL	0x25		/* data abort from the same EL */
#define ESR_EC_AL_SP		0x26		/* misaligned SP */
#define ESR_EC_FP_EXC		0x2c		/* FP exception */
#define ESR_EC_SERROR		0x2f		/* SError */
#define ESR_EC_BREAKPT_LOWER_EL	0x30		/* hardware breakpoint from lower EL */
#define ESR_EC_BREAKPT_SAME_EL	0x31		/* hardware breakpoint from the same EL */
#define ESR_EC_SS_LOWER_EL	0x32		/* software single step from lower EL */
#define ESR_EC_SS_SAME_EL	0x33		/* software single step from the same EL */
#define ESR_EC_WATCHPT_LOWER_EL	0x34		/* hardware watchpoint from lower EL */
#define ESR_EC_WATCHPT_SAME_EL	0x35		/* hardware watchpoint from the same EL */
#define ESR_EC_BRK		0x3c		/* BRK */

#define ESR_WF_TI(esr)		((esr) & 0x3)	/* WF* trapped instruction */

#define ESR_WF_TI_WFI		0x0
#define ESR_WF_TI_WFE		0x1
#define ESR_WF_TI_WFIT		0x2
#define ESR_WF_TI_WFET		0x3

#define ESR_SVC_IMM(esr)	((esr) & 0xffff)
#define ESR_HVC_IMM(esr)	((esr) & 0xffff)
#define ESR_SMC_IMM(esr)	((esr) & 0xffff)
#define ESR_PAC_INFO(esr)	((esr) & 0x3)

#define ESR_IABT_IFSC(esr)	((esr) & 0x3f)	/* instruction fault status code */

#define ESR_IABT_IFSC_PERM_L0	0x0c		/* permission fault, level 0 */
#define ESR_IABT_IFSC_PERM_L1	0x0d		/* permission fault, level 1 */
#define ESR_IABT_IFSC_PERM_L2	0x0e		/* permission fault, level 2 */
#define ESR_IABT_IFSC_PERM_L3	0x0f		/* permission fault, level 3 */
#define ESR_IABT_IFSC_SYNC_EXT	0x10		/* synchronous external abort */

#define ESR_DABT_DFSC(esr)	((esr) & 0x3f)	/* data fault status code */

#define ESR_DABT_DFSC_PERM_L0	0x0c		/* permission fault, level 0 */
#define ESR_DABT_DFSC_PERM_L1	0x0d		/* permission fault, level 1 */
#define ESR_DABT_DFSC_PERM_L2	0x0e		/* permission fault, level 2 */
#define ESR_DABT_DFSC_PERM_L3	0x0f		/* permission fault, level 3 */
#define ESR_DABT_DFSC_MTE	0x11		/* synchronous MTE tag check fault */
#define ESR_DABT_DFSC_AL	0x21		/* alignment fault */

#define ESR_WNR			0x040		/* "write, not read" bit */
#define ESR_CM			0x100		/* it was a cache maintenance operation */

#define ESR_ABT_FNV		0x400		/* "FAR not valid" bit (both IABT & DABT) */

#define ESR_FP_EXC_TFV(esr)	((esr) & 0x800000) /* other FP bits hols meaningful values */
#define ESR_FP_EXC_IDF(esr)	((esr) & 0x80)	/* input denormal */
#define ESR_FP_EXC_IXF(esr)	((esr) & 0x10)	/* inexact */
#define ESR_FP_EXC_UFF(esr)	((esr) & 0x08)	/* underflow */
#define ESR_FP_EXC_OFF(esr)	((esr) & 0x04)	/* overflow */
#define ESR_FP_EXC_DZF(esr)	((esr) & 0x02)	/* divide by zero */
#define ESR_FP_EXC_IOF(esr)	((esr) & 0x01)	/* invalid operation */

#define ESR_BTI_BTYPE(esr)	((esr) & 0x3)	/* BTYPE that caused the BTI exception */

#define ESR_SS_ISV(esr)		((esr) & 0x1000000) /* EX holds a meaningful value */
#define ESR_SS_EX(esr)		((esr) & 0x40)	/* stepped instruction was load-exclusive */

#define ESR_WATCHPT_DFSC	0x22

#define ESR_BRK_IMM(esr)	((esr) & 0xffff)

#endif /* _AARCH64_BITS_ESR_ */
