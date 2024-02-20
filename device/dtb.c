#include <device/dtb.h>

#define DTB_MAGIC	0xd00dfeed

static dtb_t global_dtb;

static uint32_t be32toh(uint32_t arg) {
	/* Assumes little-endian.  */
	return __builtin_bswap32(arg);
}

kern_return_t dtb_load(dtb_t dtb) {
	global_dtb = dtb;

	if (be32toh(dtb->magic) != DTB_MAGIC)
		return KERN_INVALID_VALUE;

	return KERN_SUCCESS;
}
