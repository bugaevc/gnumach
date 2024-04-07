#ifndef _AARCH64_BITS_PTE_
#define _AARCH64_BITS_PTE_

/* PTE bits */
#define AARCH64_PTE_ADDR_MASK	0x0000fffffffff000UL
#define AARCH64_PTE_PROT_MASK	0x00600000000000c0UL

/* Block or table */
#define AARCH64_PTE_BLOCK	0x0000000000000000UL	/* points to a block of phys memory */
#define AARCH64_PTE_TABLE	0x0000000000000002UL	/* points to a next level table */
#define AARCH64_PTE_LEVEL3	0x0000000000000002UL	/* this is a level 3 PTE (same value as table) */

#define AARCH64_PTE_VALID	0x0000000000000001UL	/* this entry is valid */
#define AARCH64_PTE_NS		0x0000000000000020UL	/* security bit (only EL3 & secure EL1) */
#define AARCH64_PTE_ACCESS	0x0000000000000400UL	/* if unset, trap on access */
#define AARCH64_PTE_NG		0x0000000000000800UL	/* tag TLB entries with ASID */
#define AARCH64_PTE_BTI		0x0004000000000000UL	/* enable branch target identification */
#define AARCH64_PTE_CONTIG	0x0010000000000000UL	/* hint that this is a part of contigous set */
#define AARCH64_PTE_PXN		0x0020000000000000UL	/* privileged execute never */
#define AARCH64_PTE_UXN		0x0040000000000000UL	/* unprivileged execute never */

#define AARCH64_PTE_MAIR_INDEX(i) ((i) << 2)		/* cache policies, as an index into MAIR table */

/* Access permissions */
#define AARCH64_PTE_EL0_ACCESS	0x0000000000000040UL	/* EL0 can access (read or write, subject to READ_ONLY) */
#define AARCH64_PTE_READ_ONLY	0x0000000000000080UL	/* can not be written */

/* Shareability */
#define AARCH64_PTE_NON_SH	0x0000000000000000UL	/* non-shareable */
#define AARCH64_PTE_OUTER_SH	0x0000000000000200UL	/* outer shareable */
#define AARCH64_PTE_INNER_SH	0x0000000000000300UL	/* inner shareable */

#endif /* _AARCH64_BITS_PTE_ */
