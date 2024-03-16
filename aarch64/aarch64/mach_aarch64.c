#include "aarch64/mach_aarch64.server.h"
#include "aarch64/hwcaps.h"
#include <kern/host.h>
#include <string.h>

kern_return_t aarch64_get_hwcaps(
	const host_t		host,
	uint32_t		*out_hwcaps,
	mach_msg_type_number_t	*hwcapsCnt,
	uint64_t		*midr_el1,
	uint64_t		*revidr_el1)
{
	uint64_t	v;

	if (host != &realhost)
		return KERN_INVALID_HOST;

	*hwcapsCnt = MIN(*hwcapsCnt, HWCAPS_COUNT);
	memcpy(out_hwcaps, hwcaps, sizeof(uint32_t) * (*hwcapsCnt));

	asm("mrs %0, midr_el1" : "=r"(v));
	*midr_el1 = v;
	asm("mrs %0, revidr_el1" : "=r"(v));
	*revidr_el1 = v;

	return KERN_SUCCESS;
}
