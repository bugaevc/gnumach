#ifndef _AARCH64_BITS_SCTLR_
#define _AARCH64_BITS_SCTLR_

#define SCTLR_M			0x0000000000000001UL	/* enable MMU */
#define SCTLR_A			0x0000000000000002UL	/* enable alignment checking */
#define SCTLR_SA		0x0000000000000008UL	/* enable SP alignment checking in EL1 */
#define SCTLR_SA0		0x0000000000000010UL	/* enable SP alignment checking in EL0 */
#define SCTLR_ENDB		0x0000000000002000UL	/* PAC */
#define SCTLR_UCT		0x0000000000008000UL	/* allow EL0 to access CTR_EL0 */
#define SCTLR_SPAN		0x0000000000800000UL	/* don't set psate.PAN upon an exception to EL1 */
#define SCTLR_UCI		0x0000000004000000UL	/* allow EL0 to issue cache maintenance instructions */
#define SCTLR_ENDA		0x0000000008000000UL	/* PAC */
#define SCTLR_ENIB		0x0000000040000000UL	/* PAC */
#define SCTLR_ENIA		0x0000000080000000UL	/* PAC */
#define SCTLR_BT0		0x0000000800000000UL	/* PACIASP/PACIBSP does not act like BTI JC in EL0 */
#define SCTLR_BT1		0x0000001000000000UL	/* PACIASP/PACIBSP does not act like BTI JC in EL1 */
#define SCTLR_SSBS		0x0000100000000000UL	/* set SSBS to 1 on exception to EL1 (otherwise, to 0) */
#define SCTLR_EPAN		0x0200000000000000UL	/* enable EPAN */

#endif /* _AARCH64_BITS_SCTLR_ */
