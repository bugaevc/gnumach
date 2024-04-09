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

#include <device/conf.h>
#include <kern/mach_clock.h>
#include "aarch64/model_dep.h"
#ifdef MACH_KMSG
#include <device/kmsg.h>
#endif
#include <device/ramdisk.h>

struct dev_ops	dev_name_list[] =
{
	{ "cn",		nulldev_open,	nulldev_close,	nulldev_read,
	  nulldev_write,	nulldev_getstat,	nulldev_setstat,	nomap,
	  nodev_async_in,	nulldev_reset,	nulldev_portdeath,	0,
	  nodev_info},
	{ "time",	timeopen,	timeclose,	nulldev_read,
	  nulldev_write,	nulldev_getstat,	nulldev_setstat,	timemmap,
	  nodev_async_in,	nulldev_reset,	nulldev_portdeath,	0,
	  nodev_info},
	{ "mem",	nulldev_open,	nulldev_close,	nulldev_read,
	  nulldev_write,	nulldev_getstat,	nulldev_setstat,	memmmap,
	  nodev_async_in,	nulldev_reset,	nulldev_portdeath,	0,
	  nodev_info },
#ifdef MACH_KMSG
	{ "kmsg",	kmsgopen,	kmsgclose,	kmsgread,
	  nulldev_write,	kmsggetstat,	nulldev_setstat,	nomap,
	  nodev_async_in,	nulldev_reset,	nulldev_portdeath,	0,
	  nodev_info },
#endif
	RAMDISK_DEV_OPS,
};
int	dev_name_count = sizeof(dev_name_list) / sizeof(dev_name_list[0]);

struct dev_indirect dev_indirect_list[] =
{
	{ "console",	&dev_name_list[0],	0 }
};
int	dev_indirect_count = sizeof(dev_indirect_list) / sizeof(dev_indirect_list[0]);
