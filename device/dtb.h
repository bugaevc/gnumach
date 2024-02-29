#ifndef _DEVICE_DTB_H_
#define _DEVICE_DTB_H_

#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/machine/vm_types.h>
#include <device/device_types.h>

#define DTB_SENTINEL_OFFSET		((vm_offset_t) -1)
#define DTB_IS_SENTINEL(s)		((s).offset == DTB_SENTINEL_OFFSET)

/*
 *	Big endian 4-byte integer.
 */
typedef struct {
	unsigned char	bytes[4];
} dtb_uint32_t;

struct dtb_header {
	dtb_uint32_t	magic;
	dtb_uint32_t	total_size;
	dtb_uint32_t	offset_dt_struct;
	dtb_uint32_t	offset_dt_strings;
	dtb_uint32_t	offset_mem_rsvmap;
	dtb_uint32_t	version;
	dtb_uint32_t	last_compatible_version;
	dtb_uint32_t	boot_cpuid_phys;
	dtb_uint32_t	sizeof_dt_strings;
	dtb_uint32_t	sizeof_dt_struct;
};

typedef const struct dtb_header *dtb_t;

extern kern_return_t dtb_load(dtb_t dtb);

struct dtb_node {
	vm_offset_t	offset;
	const char	*name;
	unsigned short	address_cells;
	unsigned short	size_cells;
};

struct dtb_prop {
	vm_offset_t	offset;
	const char	*name;
	const void	*data;
	vm_size_t	length;
};

extern struct dtb_node dtb_root_node(void);
extern struct dtb_prop dtb_node_first_prop(const struct dtb_node*);
extern struct dtb_prop dtb_node_next_prop(const struct dtb_prop*);
extern struct dtb_node dtb_node_first_child(const struct dtb_node*);
extern struct dtb_node dtb_node_next_sibling(const struct dtb_node*);

#define dtb_for_each_child(parent, child)				\
	for (child = dtb_node_first_child(&(parent));			\
	     !DTB_IS_SENTINEL(child);					\
	     child = dtb_node_next_sibling(&child))

#define dtb_for_each_prop(node, prop)					\
	for (prop = dtb_node_first_prop(&(node));			\
	     !DTB_IS_SENTINEL(prop);					\
	     prop = dtb_node_next_prop(&prop))

extern struct dtb_node dtb_node_by_path(const char *node_path);
extern struct dtb_prop dtb_node_find_prop(const struct dtb_node*, const char *prop_name);
extern boolean_t dtb_node_is_compatible(const struct dtb_node*, const char *model);

extern uint64_t dtb_prop_read_cells(
	const struct dtb_prop	*prop,
	unsigned short		size,
	vm_size_t		off);

#endif /* _DEVICE_DTB_H_ */
