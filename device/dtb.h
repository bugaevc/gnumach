#ifndef _DEVICE_DTB_H_
#define _DEVICE_DTB_H_

#include <mach/kern_return.h>
#include <stdint.h>

struct dtb_header {
	uint32_t magic;
	uint32_t total_size;
	uint32_t offset_dt_struct;
	uint32_t offset_dt_strings;
	uint32_t offset_mem_rsvmap;
	uint32_t version;
	uint32_t last_compatible_version;
	uint32_t boot_cpuid_phys;
	uint32_t sizeof_dt_strings;
	uint32_t sizeof_dt_struct;
};

typedef const struct dtb_header *dtb_t;

extern kern_return_t dtb_load(dtb_t dtb);

#endif /* _DEVICE_DTB_H_ */
