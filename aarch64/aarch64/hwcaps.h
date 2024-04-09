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

#include <mach/machine/mach_aarch64_types.h>

extern uint32_t	hwcaps[HWCAPS_COUNT];

extern uint32_t	hwcap_internal;

#define HWCAP_INT_PAN			0x01		/* privileged access never */
#define HWCAP_INT_EPAN			0x02		/* extended privileged access never */
#define HWCAP_INT_ASID16		0x04		/* 16-bit ASID */
#define HWCAP_INT_UAO			0x08		/* user access override */
#define HWCAP_INT_NV2			0x10		/* nested virtualization v2 */

extern void	hwcaps_init(void);
