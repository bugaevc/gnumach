#ifndef _AARCH64_BITS_SPSR_
#define _AARCH64_BITS_SPSR_

#define SPSR_SPSEL(spsr)	((spsr) & 0x1)	/* select sp: */
#define SPSR_SPSEL_0		0x00000000	/* ...sp = SP_EL0 */
#define SPSR_SPSEL_N		0x00000001	/* ...sp = SP_ELn */

#define SPSR_EL(spsr)		(((spsr) & 0xc) >> 2)	/* exception level, 0 to 3 */
#define SPSR_MAKE_EL(el)	((el) << 2)

#define SPSR_NRW(spsr)		((spsr) & 0x10)	/* "not register width": */
#define SPSR_NRW_64		0x00000000	/* ...AArch64 */
#define SPSR_NRW_32		0x00000010	/* ...AArch32 */
				/* a reserved bit here */
#define SPSR_F			0x00000040	/* FIQ masked */
#define SPSR_I			0x00000080	/* IRQ masked */
#define SPSR_A			0x00000100	/* SError masked */
#define SPSR_D			0x00000200	/* debug exceptions masked */
#define SPSR_DAIF		(SPSR_D | SPSR_A | SPSR_I | SPSR_F)
#define SPSR_AIF		(SPSR_A | SPSR_I | SPSR_F)

#define SPSR_BTYPE_MASK		0x00000c00	/* branch type indicator (BTI) */

#define SPSR_SSBS		0x00001000	/* speculative store bypass safe */
#define SPSR_ALLINT		0x00002000	/* IRQ and FIQ masked */
				/* reserved bits here */
#define SPSR_IL			0x00100000	/* illegal execution state */
#define SPSR_SS			0x00200000	/* software single step */
#define SPSR_PAN		0x00400000	/* privileged access never */
#define SPSR_UAO		0x00800000	/* user access override */
#define SPSR_DIT		0x01000000	/* data independent timing */
#define SPSR_TCO		0x02000000	/* tag check override (MTE) */
				/* reserved bits here */
#define SPSR_V			0x10000000	/* overflow condition */
#define SPSR_C			0x20000000	/* carry condition */
#define SPSR_Z			0x40000000	/* zero condition */
#define SPSR_N			0x80000000	/* negative condition */
#define SPSR_NZCV		(SPSR_N | SPSR_Z | SPSR_C | SPSR_V)

#define SPSR_RES0		0xffffffff080fc020

#endif /* _AARCH64_BITS_SPSR_ */
