#include "aarch64/mach_aarch64.server.h"

kern_return_t aarch64_get_hwcaps(
	const host_t		host,
	hwcaps_t		hwcaps,
	mach_msg_type_number_t	*hwcapsCnt,
	uint64_t		*midr_el1,
	uint64_t		*revidr_el1)
{
	return KERN_INVALID_ADDRESS;
}
