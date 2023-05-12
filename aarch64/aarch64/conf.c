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
