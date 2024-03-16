#include <mach/machine/mach_aarch64_types.h>

extern uint32_t	hwcaps[HWCAPS_COUNT];

extern uint32_t	hwcap_internal;

#define HWCAP_INT_EPAN			0x1		/* extended privileged access never */
#define HWCAP_INT_ASID16		0x2		/* 16-bit ASID */

extern void	hwcaps_init(void);
