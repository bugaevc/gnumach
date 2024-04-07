#ifndef _AARCH64_BITS_ID_AA64_
#define _AARCH64_BITS_ID_AA64_

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

#define ID_AA64PFR1_BT(v)		((v) & 0xf)
#define ID_AA64PFR1_SSBS(v)		(((v) >> 4) & 0xf)
#define ID_AA64PFR1_SME(v)		(((v) >> 24) 0xf)

#define ID_AA64PFR1_BT_NONE		0x0			/* BTI not supported */
#define ID_AA64PFR1_BT_BTI		0x1			/* BTI supported */
#define ID_AA64PFR1_SSBS_NONE		0x0			/* SSBS not supported */
#define ID_AA64PFR1_SSBS_SSBS		0x1			/* SSBS supported */
#define ID_AA64PFR1_SSBS_SSBS2		0x2			/* SSBS + SSBS2 supported */
#define ID_AA64PFR1_SME_NONE		0x0			/* SME not supported */
#define ID_AA64PFR1_SME_SME		0x1			/* SME supported */
#define ID_AA64PFR1_SME_SME2		0x2			/* SME + SME2 supported */


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
#define ID_AA64ISAR0_TS(v)		(((v) >> 52) & 0xf)
#define ID_AA64ISAR0_RNDR(v)		(((v) >> 60) & 0xf)

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
#define ID_AA64ISAR0_TS_NONE		0x0			/* FLAGM not supported */
#define ID_AA64ISAR0_TS_FLAGM		0x1			/* FLAGM supported */
#define ID_AA64ISAR0_TS_FLAGM2		0x2			/* FLAGM + FLAGM2 supported */
#define ID_AA64ISAR0_RNDR_NONE		0x0			/* RNDR not supported */
#define ID_AA64ISAR0_RNDR_RNDR		0x1			/* RNDR supported */

#define ID_AA64ISAR1_DPB(v)		((v) & 0xf)
#define ID_AA64ISAR1_APA(v)		(((v) >> 4) & 0xf)
#define ID_AA64ISAR1_API(v)		(((v) >> 8) & 0xf)
#define ID_AA64ISAR1_JSCVT(v)		(((v) >> 12) & 0xf)
#define ID_AA64ISAR1_FCMA(v)		(((v) >> 16) & 0xf)
#define ID_AA64ISAR1_LRCPC(v)		(((v) >> 20) & 0xf)
#define ID_AA64ISAR1_GPA(v)		(((v) >> 24) & 0xf)
#define ID_AA64ISAR1_GPI(v)		(((v) >> 28) & 0xf)
#define ID_AA64ISAR1_FRINTTS(v)		(((v) >> 32) & 0xf)
#define ID_AA64ISAR1_SB(v)		(((v) >> 36) & 0xf)
#define ID_AA64ISAR1_BF16(v)		(((v) >> 44) & 0xf)
#define ID_AA64ISAR1_DGH(v)		(((v) >> 48) & 0xf)
#define ID_AA64ISAR1_I8MM(v)		(((v) >> 52) & 0xf)

#define ID_AA64ISAR1_DPB_NONE		0x0			/* DPB not supported */
#define ID_AA64ISAR1_DPB_DPB		0x1			/* DPB supported */
#define ID_AA64ISAR1_DPB_DPB2		0x2			/* DPB + DPB2 supported */
#define ID_AA64ISAR1_APA_NONE		0x0			/* PAC w/ QARMA5 not supported */
#define ID_AA64ISAR1_APA_APA		0x1			/* PAC w/ QARMA5 supported */
#define ID_AA64ISAR1_API_NONE		0x0			/* PAC w/ implementation-defined algo not supported */
#define ID_AA64ISAR1_API_API		0x1			/* PAC w/ implementation-defined algo supported */
#define ID_AA64ISAR1_JSCVT_NONE		0x0			/* JSCVT not supported */
#define ID_AA64ISAR1_JSCVT_JSCVT	0x1			/* JSCVT supported */
#define ID_AA64ISAR1_FCMA_NONE		0x0			/* FCMA not supported */
#define ID_AA64ISAR1_FCMA_FCMA		0x1			/* FCMA supported */
#define ID_AA64ISAR1_LRCPC_NONE		0x0			/* LRCPC not supported */
#define ID_AA64ISAR1_LRCPC_LRCPC	0x1			/* LRCPC supported */
#define ID_AA64ISAR1_LRCPC_LRCPC2	0x2			/* LRCPC + LRCPC2 supported */
#define ID_AA64ISAR1_LRCPC_LRCPC3	0x3			/* LRCPC + LRCPC2 + LRCPC3 supported */
#define ID_AA64ISAR1_GPA_NONE		0x0			/* PAC/GA w/ QARMA5 not supported */
#define ID_AA64ISAR1_GPA_GPA		0x1			/* PAC/GA w/ QARMA5 supported */
#define ID_AA64ISAR1_GPI_NONE		0x0			/* PAC/GA w/ implementation-defined algo not supported */
#define ID_AA64ISAR1_GPI_GPI		0x1			/* PAC/GA w/ implementation-defined algo supported */
#define ID_AA64ISAR1_FRINTTS_NONE	0x0			/* FRINT* not supported */
#define ID_AA64ISAR1_FRINTTS_FRINTTS	0x1			/* FRINT* supported */
#define ID_AA64ISAR1_SB_NONE		0x0			/* SB not supported */
#define ID_AA64ISAR1_BF16_NONE		0x0			/* BFloat16 not supported */
#define ID_AA64ISAR1_BF16_BF16		0x1			/* BFloat16 supported */
#define ID_AA64ISAR1_BF16_EBF16		0x2			/* BFloat16 + EBF supported */
#define ID_AA64ISAR1_DGH_NONE		0x0			/* DGH not supported */
#define ID_AA64ISAR1_DGH_DGH		0x1			/* DGH supported */
#define ID_AA64ISAR1_I8MM_NONE		0x0			/* Int8 matrix multiplication not supported */
#define ID_AA64ISAR1_I8MM_I8MM		0x1			/* Int8 matrix multiplication supported */

#define ID_AA64MMFR1_ASID(v)		(((v) >> 4) & 0xf)
#define ID_AA64MMFR1_PAN(v)		(((v) >> 20) & 0xf)

#define ID_AA64MMFR1_ASID_8		0x0			/* 8-bit ASID */
#define ID_AA64MMFR1_ASID_16		0x2			/* 16-bit ASID */
#define ID_AA64MMFR1_PAN_NONE		0x0			/* PAN not supported */
#define ID_AA64MMFR1_PAN_PAN		0x1			/* PAN supported */
#define ID_AA64MMFR1_PAN_PAN2		0x2			/* PAN + PAN2 supported */
#define ID_AA64MMFR1_PAN_PAN3		0x3			/* PAN + PAN2 + PAN3 supported */

#define ID_AA64MMFR2_UAO(v)		(((v) >> 4) & 0xf)
#define ID_AA64MMFR2_AT(v)		(((v) >> 32) & 0xf)

#define ID_AA64MMFR2_UAO_NONE		0x0			/* UAO not supported */
#define ID_AA64MMFR2_UAO_UAO		0x1			/* UAO supported */
#define ID_AA64MMFR2_AT_NONE		0x0			/* AT not supported */
#define ID_AA64MMFR2_AT_AT		0x1			/* AT supported */

#endif /* _AARCH64_BITS_ID_AA64_ */
