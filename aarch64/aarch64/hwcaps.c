#include "aarch64/hwcaps.h"
#include "aarch64/bits/id_aa64.h"

/* https://docs.kernel.org/arch/arm64/elf_hwcaps.html */

uint32_t	hwcaps[HWCAPS_COUNT];
uint32_t	hwcap_internal;

void hwcaps_init(void)
{
	uint64_t	id_aa64pfr0;
	uint64_t	id_aa64pfr1;
	uint64_t	id_aa64isar0;
	uint64_t	id_aa64isar1;
	uint64_t	id_aa64mmfr1;
	uint64_t	id_aa64mmfr2;

	asm("mrs %0, id_aa64pfr0_el1" : "=r"(id_aa64pfr0));
	asm("mrs %0, id_aa64pfr1_el1" : "=r"(id_aa64pfr1));
	asm("mrs %0, id_aa64isar0_el1" : "=r"(id_aa64isar0));
	asm("mrs %0, id_aa64isar1_el1" : "=r"(id_aa64isar1));
	asm("mrs %0, id_aa64mmfr1_el1" : "=r"(id_aa64mmfr1));
	asm("mrs %0, id_aa64mmfr2_el1" : "=r"(id_aa64mmfr2));

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
	/*
	if (ID_AA64PFR0_SVE(id_aa64pfr0) != ID_AA64PFR0_SVE_NONE)
		hwcaps[0] |= HWCAP_SVE;
	*/
	if (ID_AA64ISAR0_FHM(id_aa64isar0) != ID_AA64ISAR0_FHM_NONE)
		hwcaps[0] |= HWCAP_ASIMDFHM;
	if (ID_AA64PFR0_DIT(id_aa64pfr0) != ID_AA64PFR0_DIT_NONE)
		hwcaps[0] |= HWCAP_DIT;
	if (ID_AA64MMFR2_AT(id_aa64mmfr2) != ID_AA64MMFR2_AT_NONE)
		hwcaps[0] |= HWCAP_USCAT;
	if (ID_AA64ISAR0_TS(id_aa64isar0) == ID_AA64ISAR0_TS_FLAGM)
		hwcaps[0] |= HWCAP_FLAGM;
	else if (ID_AA64ISAR0_TS(id_aa64isar0) == ID_AA64ISAR0_TS_FLAGM2) {
		hwcaps[0] |= HWCAP_FLAGM;
		hwcaps[1] |= HWCAP2_FLAGM2;
	}
	if (ID_AA64PFR1_SSBS(id_aa64pfr1) != ID_AA64PFR1_SSBS_NONE)
		hwcaps[0] |= HWCAP_SSBS;
	if (ID_AA64ISAR1_SB(id_aa64isar1) != ID_AA64ISAR1_SB_NONE)
		hwcaps[0] |= HWCAP_SB;
	if (ID_AA64ISAR1_APA(id_aa64isar1) != ID_AA64ISAR1_APA_NONE)
		hwcaps[0] |= HWCAP_PACA;
	if (ID_AA64ISAR1_API(id_aa64isar1) != ID_AA64ISAR1_API_NONE)
		hwcaps[0] |= HWCAP_PACA;
	if (ID_AA64ISAR1_GPA(id_aa64isar1) != ID_AA64ISAR1_GPA_NONE)
		hwcaps[0] |= HWCAP_PACG;
	if (ID_AA64ISAR1_GPI(id_aa64isar1) != ID_AA64ISAR1_GPI_NONE)
		hwcaps[0] |= HWCAP_PACG;

	/*
	 *	SME/SVE are not exposed to userland for now
	 *	even if hardware supports them, due to lack
	 *	of kernels-side support.
	 */

	if (ID_AA64ISAR1_FRINTTS(id_aa64isar1) != ID_AA64ISAR1_FRINTTS_NONE)
		hwcaps[1] |= HWCAP2_FRINT;
	if (ID_AA64ISAR1_I8MM(id_aa64isar1) != ID_AA64ISAR1_I8MM_NONE)
		hwcaps[1] |= HWCAP2_I8MM;
	if (ID_AA64ISAR1_BF16(id_aa64isar1) == ID_AA64ISAR1_BF16_BF16)
		hwcaps[1] |= HWCAP2_BF16;
	else if (ID_AA64ISAR1_BF16(id_aa64isar1) == ID_AA64ISAR1_BF16_EBF16)
		hwcaps[1] |= HWCAP2_BF16 | HWCAP2_EBF16;
	if (ID_AA64ISAR1_DGH(id_aa64isar1) != ID_AA64ISAR1_DGH_NONE)
		hwcaps[1] |= HWCAP2_DGH;
	if (ID_AA64ISAR0_RNDR(id_aa64isar0) != ID_AA64ISAR0_RNDR_NONE)
		hwcaps[1] |= HWCAP2_RNG;
	if (ID_AA64PFR1_BT(id_aa64pfr1) != ID_AA64PFR1_BT_NONE)
		hwcaps[1] |= HWCAP2_BTI;

	/* to be continued... */

	if (ID_AA64MMFR1_PAN(id_aa64mmfr1) != ID_AA64MMFR1_PAN_NONE) {
		/*
		 *	PAN is supported, enable it.
		 */
		asm volatile(".word 0xd500419f");	/* msr PAN, #1 */
		hwcap_internal |= HWCAP_INT_PAN;
	}
	if (ID_AA64MMFR1_PAN(id_aa64mmfr1) >= ID_AA64MMFR1_PAN_PAN3)
		hwcap_internal |= HWCAP_INT_EPAN;
	if (ID_AA64MMFR1_ASID(id_aa64mmfr1) == ID_AA64MMFR1_ASID_16)
		hwcap_internal |= HWCAP_INT_ASID16;
	if (ID_AA64MMFR2_UAO(id_aa64mmfr2) != ID_AA64MMFR2_UAO_NONE)
		hwcap_internal |= HWCAP_INT_UAO;
}
