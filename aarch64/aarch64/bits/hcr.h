#ifndef _AARCH64_BITS_HCR_
#define _AARCH64_BITS_HCR_

#define HCR_TWI			0x0000000000002000	/* trap WFI to EL2 */
#define HCR_TWE			0x0000000000004000	/* trap WFE to EL2 */
#define HCR_TGE			0x0000000008000000	/* trap general exceptions from EL0 to EL2 */
#define HCR_RW			0x0000000080000000	/* register width, EL1 is AArch64 if set */
#define HCR_E2H			0x0000000400000000	/* EL2 host */
#define HCR_VSE			0x0000000000000100	/* virtual SError pending */
#define HCR_VI			0x0000000000000080	/* virtual IRQ pending */
#define HCR_VF			0x0000000000000040	/* virtual FIQ pending */
#define HCR_AMO			0x0000000000000020	/* physical SError rounting (?) */
#define HCR_IMO			0x0000000000000010	/* physical IRQ routing (?) */
#define HCR_FMO			0x0000000000000008	/* physical FIQ routing (?) */
#define HCR_VM			0x0000000000000001	/* enable virtualization (stage 2 translation) */

#endif /* _AARCH64_BITS_HCR_ */
