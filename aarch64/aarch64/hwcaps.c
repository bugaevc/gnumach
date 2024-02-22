#include "aarch64/hwcaps.h"

#define ID_AA64PFR0_ASIMD(v)		(((v) >> 20) & 0xf)
#define ID_AA64PFR0_FP(v)		(((v) >> 16) & 0xf)
#define ID_AA64PFR0_SVE(v)		(((v) >> 32) & 0xf)
#define ID_AA64PFR0_DIT(v)		(((v) >> 48) & 0xf)

#define ID_AA64PFR0_ASIMD_NONE		0xf			/* AdvSIMD not supported */
#define ID_AA64PFR0_ASIMD_ASIMD		0x0			/* AdvSIMD supported */
#define ID_AA64PFR0_ASIMD_FP16		0x1			/* AdvSIMD + FP16/FPHP supported */
#define ID_AA64PFR0_FP_NONE		0xf			/* FP not supported */
#define ID_AA64PFR0_FP_FP		0x0			/* FP supported */
#define ID_AA64PFR0_FP_FP16		0x1			/* FP + FP16/FPHP supported */
#define ID_AA64PFR0_SVE_NONE		0x0			/* SVE not supported */
#define ID_AA64PFR0_SVE_SVE		0x1			/* SVE supported */
#define ID_AA64PFR0_DIT_NONE		0x0			/* DIT not supported */
#define ID_AA64PFR0_DIT_DIT		0x1			/* DIT supported */

#define ID_AA64ISAR0_AES(v)		(((v) >> 4) & 0xf)
#define ID_AA64ISAR0_SHA1(v)		(((v) >> 8) & 0xf)
#define ID_AA64ISAR0_SHA2(v)		(((v) >> 12) & 0xf)
#define ID_AA64ISAR0_CRC32(v)		(((v) >> 16) & 0xf)
#define ID_AA64ISAR0_ATOMIC(v)		(((v) >> 20) & 0xf)
#define ID_AA64ISAR0_RDM(v)		(((v) >> 28) & 0xf)
#define ID_AA64ISAR0_SHA3(v)		(((v) >> 32) & 0xf)
#define ID_AA64ISAR0_SM3(v)		(((v) >> 36) & 0xf)
#define ID_AA64ISAR0_SM4(v)		(((v) >> 40) & 0xf)
#define ID_AA64ISAR0_DP(v)		(((v) >> 44) & 0xf)
#define ID_AA64ISAR0_FHM(v)		(((v) >> 48) & 0xf)

#define ID_AA64ISAR0_AES_AES		0x1			/* AES supported */
#define ID_AA64ISAR0_AES_PMULL		0x2			/* AES + PMULL supported */
#define ID_AA64ISAR0_SHA1_NONE		0x0			/* SHA1 not supported */
#define ID_AA64ISAR0_SHA2_NONE		0x0			/* SHA2 not supported */
#define ID_AA64ISAR0_SHA2_SHA2		0x1			/* SHA2 supported */
#define ID_AA64ISAR0_SHA2_SHA512	0x2			/* SHA2 + SHA512 supported */
#define ID_AA64ISAR0_CRC32_NONE		0x0			/* CRC32 not supported */
#define ID_AA64ISAR0_ATOMIC_NONE	0x0			/* atomics not supported */
#define ID_AA64ISAR0_ATOMIC_LSE		0x2			/* LSE supported */
#define ID_AA64ISAR0_ATOMIC_LSE128	0x3			/* LSE + LSE128 supported */
#define ID_AA64ISAR0_RDM_NONE		0x0			/* RDM not supported */
#define ID_AA64ISAR0_RDM_RDM		0x1			/* RDM supported */
#define ID_AA64ISAR0_SHA3_NONE		0x0			/* SHA3 not supported */
#define ID_AA64ISAR0_SHA3_SHA3		0x1			/* SHA3 supported */
#define ID_AA64ISAR0_SM3_NONE		0x0			/* SM3 not supported */
#define ID_AA64ISAR0_SM3_SM3		0x1			/* SM3 supported */
#define ID_AA64ISAR0_SM4_NONE		0x0			/* SM4 not supported */
#define ID_AA64ISAR0_SM4_SM4		0x1			/* SM4 supported */
#define ID_AA64ISAR0_DP_NONE		0x0			/* DP not supported */
#define ID_AA64ISAR0_DP_DP		0x1			/* DP supported */
#define ID_AA64ISAR0_FHM_NONE		0x0			/* FHM not supported */
#define ID_AA64ISAR0_FHM_FHM		0x1			/* FHM supported */

#define ID_AA64ISAR1_DPB(v)		((v) & 0xf)
#define ID_AA64ISAR1_JSCVT(v)		(((v) >> 12) & 0xf)
#define ID_AA64ISAR1_FCMA(v)		(((v) >> 16) & 0xf)
#define ID_AA64ISAR1_LRCPC(v)		(((v) >> 20) & 0xf)

#define ID_AA64ISAR1_DPB_NONE		0x0			/* DPB not supported */
#define ID_AA64ISAR1_DPB_DPB		0x1			/* DPB supported */
#define ID_AA64ISAR1_DPB_DPB2		0x2			/* DPB + DPB2 supported */
#define ID_AA64ISAR1_JSCVT_NONE		0x0			/* JSCVT not supported */
#define ID_AA64ISAR1_JSCVT_JSCVT	0x1			/* JSCVT supported */
#define ID_AA64ISAR1_FCMA_NONE		0x0			/* FCMA not supported */
#define ID_AA64ISAR1_FCMA_FCMA		0x1			/* FCMA supported */
#define ID_AA64ISAR1_LRCPC_NONE		0x0			/* LRCPC not supported */
#define ID_AA64ISAR1_LRCPC_LRCPC	0x1			/* LRCPC supported */
#define ID_AA64ISAR1_LRCPC_LRCPC2	0x2			/* LRCPC + LRCPC2 supported */
#define ID_AA64ISAR1_LRCPC_LRCPC3	0x3			/* LRCPC + LRCPC2 + LRCPC3 supported */

uint32_t	hwcaps[HWCAPS_COUNT];

void hwcaps_init(void)
{
	uint64_t	id_aa64pfr0;
	uint64_t	id_aa64isar0;
	uint64_t	id_aa64isar1;

	asm("mrs %0, id_aa64pfr0_el1" : "=r"(id_aa64pfr0));
	asm("mrs %0, id_aa64isar0_el1" : "=r"(id_aa64isar0));
	asm("mrs %0, id_aa64isar1_el1" : "=r"(id_aa64isar1));

	if (ID_AA64PFR0_FP(id_aa64pfr0) != ID_AA64PFR0_FP_NONE)
		hwcaps[0] |= HWCAP_FP;
	if (ID_AA64PFR0_FP(id_aa64pfr0) == ID_AA64PFR0_FP_FP16)
		hwcaps[0] |= HWCAP_FPHP;
	if (ID_AA64PFR0_ASIMD(id_aa64pfr0) != ID_AA64PFR0_ASIMD_NONE)
		hwcaps[0] |= HWCAP_ASIMD;
	if (ID_AA64PFR0_ASIMD(id_aa64pfr0) == ID_AA64PFR0_ASIMD_FP16)
		hwcaps[0] |= HWCAP_ASIMDHP;
	/* HWCAP_EVTSTRM? */
	if (ID_AA64ISAR0_AES(id_aa64isar0) == ID_AA64ISAR0_AES_AES)
		hwcaps[0] |= HWCAP_AES;
	else if (ID_AA64ISAR0_AES(id_aa64isar0) == ID_AA64ISAR0_AES_PMULL)
		hwcaps[0] |= HWCAP_AES | HWCAP_PMULL;
	if (ID_AA64ISAR0_SHA1(id_aa64isar0) != ID_AA64ISAR0_SHA1_NONE)
		hwcaps[0] |= HWCAP_SHA1;
	if (ID_AA64ISAR0_SHA2(id_aa64isar0) != ID_AA64ISAR0_SHA2_NONE)
		hwcaps[0] |= HWCAP_SHA2;
	if (ID_AA64ISAR0_SHA2(id_aa64isar0) == ID_AA64ISAR0_SHA2_SHA512)
		hwcaps[0] |= HWCAP_SHA512;
	if (ID_AA64ISAR0_SHA2(id_aa64isar0) != ID_AA64ISAR0_CRC32_NONE)
		hwcaps[0] |= HWCAP_CRC32;
	if (ID_AA64ISAR0_ATOMIC(id_aa64isar0) == ID_AA64ISAR0_ATOMIC_LSE)
		hwcaps[0] |= HWCAP_ATOMICS;
	else if (ID_AA64ISAR0_ATOMIC(id_aa64isar0) == ID_AA64ISAR0_ATOMIC_LSE128) {
		hwcaps[0] |= HWCAP_ATOMICS;
		hwcaps[1] |= HWCAP2_LSE128;
	}
	/* HWCAP_CPUID not set */
	if (ID_AA64ISAR0_RDM(id_aa64isar0) != ID_AA64ISAR0_RDM_NONE)
		hwcaps[0] |= HWCAP_ASIMDRDM;
	if (ID_AA64ISAR1_JSCVT(id_aa64isar1) != ID_AA64ISAR1_JSCVT_NONE)
		hwcaps[0] |= HWCAP_JSCVT;
	if (ID_AA64ISAR1_FCMA(id_aa64isar1) != ID_AA64ISAR1_FCMA_NONE)
		hwcaps[0] |= HWCAP_FCMA;
	if (ID_AA64ISAR1_LRCPC(id_aa64isar1) != ID_AA64ISAR1_LRCPC_NONE)
		hwcaps[0] |= HWCAP_LRCPC;
	if (ID_AA64ISAR1_LRCPC(id_aa64isar1) == ID_AA64ISAR1_LRCPC_LRCPC2)
		hwcaps[0] |= HWCAP_ILRCPC;
	else if (ID_AA64ISAR1_LRCPC(id_aa64isar1) == ID_AA64ISAR1_LRCPC_LRCPC3) {
		hwcaps[0] |= HWCAP_ILRCPC;
		hwcaps[1] |= HWCAP2_LRCPC3;
	}
	if (ID_AA64ISAR1_DPB(id_aa64isar1) != ID_AA64ISAR1_DPB_NONE)
		hwcaps[0] |= HWCAP_DCPOP;
	if (ID_AA64ISAR1_DPB(id_aa64isar1) != ID_AA64ISAR1_DPB_DPB2)
		hwcaps[1] |= HWCAP2_DCPODP;
	if (ID_AA64ISAR0_SHA3(id_aa64isar0) != ID_AA64ISAR0_SHA3_NONE)
		hwcaps[0] |= HWCAP_SHA3;
	if (ID_AA64ISAR0_SM3(id_aa64isar0) != ID_AA64ISAR0_SM3_NONE)
		hwcaps[0] |= HWCAP_SM3;
	if (ID_AA64ISAR0_SM4(id_aa64isar0) != ID_AA64ISAR0_SM4_NONE)
		hwcaps[0] |= HWCAP_SM4;
	if (ID_AA64ISAR0_DP(id_aa64isar0) != ID_AA64ISAR0_DP_NONE)
		hwcaps[0] |= HWCAP_ASIMDDP;
	if (ID_AA64PFR0_SVE(id_aa64pfr0) != ID_AA64PFR0_SVE_NONE)
		hwcaps[0] |= HWCAP_SVE;
	if (ID_AA64ISAR0_FHM(id_aa64isar0) != ID_AA64ISAR0_FHM_NONE)
		hwcaps[0] |= HWCAP_ASIMDFHM;
	if (ID_AA64PFR0_DIT(id_aa64pfr0) != ID_AA64PFR0_DIT_NONE)
		hwcaps[0] |= HWCAP_DIT;

	/* to be continued... */
}
