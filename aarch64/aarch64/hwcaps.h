#include <mach/machine/mach_aarch64_types.h>

extern uint32_t	hwcaps[HWCAPS_COUNT];

extern uint32_t	hwcap_internal;

#define HWCAP_INT_PAN			0x1		/* privileged access never */
#define HWCAP_INT_EPAN			0x2		/* extended privileged access never */
#define HWCAP_INT_ASID16		0x4		/* 16-bit ASID */
#define HWCAP_INT_UAO			0x8		/* user access override */

extern void	hwcaps_init(void);
