#include <mach/machine/mach_aarch64_types.h>

extern uint32_t	hwcaps[HWCAPS_COUNT];

extern uint32_t	hwcap_internal;

#define HWCAP_INT_PAN			0x01		/* privileged access never */
#define HWCAP_INT_EPAN			0x02		/* extended privileged access never */
#define HWCAP_INT_ASID16		0x04		/* 16-bit ASID */
#define HWCAP_INT_UAO			0x08		/* user access override */
#define HWCAP_INT_NV2			0x10		/* nested virtualization v2 */

extern void	hwcaps_init(void);
