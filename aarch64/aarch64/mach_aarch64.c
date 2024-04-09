/*
 * Copyright (c) 2024 Free Software Foundation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

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
