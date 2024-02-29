#include <device/dtb.h>
#include <kern/assert.h>
#include <kern/debug.h>
#include <string.h>

#define DTB_MAGIC	0xd00dfeed

#define DTB_BEGIN_NODE	0x1
#define DTB_END_NODE	0x2
#define DTB_PROP	0x3
#define DTB_NOP		0x4
#define DTB_END		0x9

static dtb_t global_dtb;
#define DTB_PTR(offset) ((const char *) global_dtb + (offset))

static uint32_t be32toh(dtb_uint32_t arg)
{
	/* Assumes little-endian.  */
	uint32_t raw;
	__builtin_memcpy(&raw, &arg, 4);
	return __builtin_bswap32(raw);
}

static const char *dt_strings_str(vm_offset_t offset)
{
	offset += be32toh(global_dtb->offset_dt_strings);
	return DTB_PTR(offset);
}

static uint32_t uint32_at_offset(vm_offset_t offset)
{
	return be32toh(*(const dtb_uint32_t *) DTB_PTR(offset));
}

static void __attribute__((noreturn)) panic_unexpected_token(vm_offset_t offset)
{
	panic("Unexpected DTB token at %#x: %#x\n", (unsigned) offset, uint32_at_offset(offset));
}

static void skip_nops(vm_offset_t *offset)
{
	while (uint32_at_offset(*offset) == DTB_NOP)
		(*offset)++;
}

static void skip_padding(vm_offset_t *offset)
{
	*offset = (*offset + 3) / 4 * 4;
}

static boolean_t skip_prop(vm_offset_t *offset)
{
	assert(((*offset) & 0x3) == 0);
	assert(uint32_at_offset(*offset) != DTB_NOP);

	if (uint32_at_offset(*offset) != DTB_PROP)
		return FALSE;
	*offset += 12 + uint32_at_offset(*offset + 4);
	return TRUE;
}

static void skip_node_header(vm_offset_t *offset)
{
	assert(((*offset) & 0x3) == 0);
	assert(uint32_at_offset(*offset) == DTB_BEGIN_NODE);

	*offset += 4;
	while (*DTB_PTR((*offset)++));		/* skip node name */
}

static struct dtb_node make_node_at_offset(vm_offset_t offset)
{
	struct dtb_node	node;

	skip_padding(&offset);
	skip_nops(&offset);

	switch (uint32_at_offset(offset)) {
		case DTB_BEGIN_NODE:
			node.offset = offset;
			node.name = DTB_PTR(node.offset) + 4;
			node.address_cells = node.size_cells = 0;
			break;

		case DTB_END_NODE:
			/*
			 *	No more (sub)nodes.
			 */
			node.offset = DTB_SENTINEL_OFFSET;
			node.name = NULL;
			node.address_cells = node.size_cells = 0;
			break;

		default:
			panic_unexpected_token(offset);
	}

	return node;
}

static struct dtb_prop make_prop_at_offset(vm_offset_t offset)
{
	struct dtb_prop	prop;

	skip_padding(&offset);
	skip_nops(&offset);

	switch (uint32_at_offset(offset)) {
		case DTB_BEGIN_NODE:
		case DTB_END_NODE:
			/*
			 *	No more properties.
			 */
			prop.offset = DTB_SENTINEL_OFFSET;
			prop.name = NULL;
			prop.data = NULL;
			prop.length = 0;
			break;

		case DTB_PROP:
			prop.offset = offset;
			prop.length = uint32_at_offset(offset + 4);
			prop.name = dt_strings_str(uint32_at_offset(offset + 8));
			prop.data = DTB_PTR(offset + 12);
			break;

		default:
			panic_unexpected_token(offset);
	}

	return prop;
}

kern_return_t dtb_load(dtb_t dtb)
{
	global_dtb = dtb;

	if (be32toh(dtb->magic) != DTB_MAGIC)
		return KERN_INVALID_VALUE;

	return KERN_SUCCESS;
}

void dtb_get_location(dtb_t *out_dtb, vm_size_t *out_dtb_size)
{
	*out_dtb = global_dtb;
	*out_dtb_size = be32toh(global_dtb->total_size);
}

struct dtb_node dtb_root_node(void)
{
	return make_node_at_offset(be32toh(global_dtb->offset_dt_struct));
}

struct dtb_prop dtb_node_first_prop(const struct dtb_node *node)
{
	vm_offset_t	offset = node->offset;

	assert(offset != DTB_SENTINEL_OFFSET);
	skip_node_header(&offset);

	return make_prop_at_offset(offset);
}

struct dtb_prop dtb_node_next_prop(const struct dtb_prop *prev_prop)
{
	boolean_t	skipped;
	vm_offset_t	offset = prev_prop->offset;

	assert(offset != DTB_SENTINEL_OFFSET);
	skipped = skip_prop(&offset);
	assert(skipped);
	return make_prop_at_offset(offset);
}

struct dtb_node dtb_node_first_child(const struct dtb_node *parent)
{
	vm_offset_t	offset = parent->offset;
	struct dtb_prop	prop;
	struct dtb_node	node;
	/*
	 *	"If missing, a client program should assume a default
	 *	value of 2 for #address-cells, and a value of 1 for
	 *	#size-cells."
	 */
	unsigned short	address_cells = 2;
	unsigned short	size_cells = 1;

	assert(offset != DTB_SENTINEL_OFFSET);

	skip_node_header(&offset);
	while (TRUE) {
		skip_padding(&offset);
		skip_nops(&offset);
		if (uint32_at_offset(offset) != DTB_PROP)
			break;
		prop = make_prop_at_offset(offset);
		if (!strcmp(prop.name, "#address-cells")) {
			assert(prop.length == 4);
			address_cells = be32toh(*(const dtb_uint32_t *) prop.data);
		} else if (!strcmp(prop.name, "#size-cells")) {
			assert(prop.length == 4);
			size_cells = be32toh(*(const dtb_uint32_t *) prop.data);
		}
		offset += 12 + prop.length;
	}

	node = make_node_at_offset(offset);
	node.address_cells = address_cells;
	node.size_cells = size_cells;
	return node;
}

struct dtb_node dtb_node_next_sibling(const struct dtb_node *node)
{
	vm_offset_t	offset = node->offset;
	unsigned int	depth = 0;
	struct dtb_node	sibling;

	assert(offset != DTB_SENTINEL_OFFSET);
	assert(uint32_at_offset(offset) == DTB_BEGIN_NODE);

	do {
		switch (uint32_at_offset(offset)) {
			case DTB_BEGIN_NODE:
				depth++;
				skip_node_header(&offset);
				break;
			case DTB_PROP:
				skip_prop(&offset);
				break;
			case DTB_END_NODE:
				offset += 4;
				depth--;
				break;
			default:
				panic_unexpected_token(offset);
		}
		skip_padding(&offset);
		skip_nops(&offset);
	} while (depth > 0);

	sibling = make_node_at_offset(offset);
	sibling.address_cells = node->address_cells;
	sibling.size_cells = node->size_cells;
	return sibling;
}

struct dtb_node dtb_node_by_path(const char *node_path)
{
	boolean_t	found;
	const char	*c = node_path + 1, *c2;
	struct dtb_node	node;

	assert(node_path[0] == '/');
	node = dtb_root_node();

	while (TRUE) {
		c2 = strchr(c, '/');
		if (c2 == NULL)
			c2 = c + strlen(c);
		found = FALSE;
		dtb_for_each_child (node, node) {
			if (!memcmp(node.name, c, c2 - c)) {
				found = TRUE;
				break;
			}
		}
		if (!found) {
			node.offset = DTB_SENTINEL_OFFSET;
			node.name = NULL;
			node.address_cells = node.size_cells = 0;
			return node;
		}
		if (*c2 == 0)
			return node;
		c = c2 + 1;
	}
}

struct dtb_prop dtb_node_find_prop(const struct dtb_node *node, const char *prop_name)
{
	struct dtb_prop	prop;

	dtb_for_each_prop (*node, prop) {
		if (!strcmp(prop.name, prop_name))
			return prop;
	}

	prop.offset = DTB_SENTINEL_OFFSET;
	prop.name = NULL;
	prop.data = NULL;
	prop.length = 0;
	return prop;
}

boolean_t dtb_node_is_compatible(const struct dtb_node *node, const char *model)
{
	struct dtb_prop prop;
	vm_size_t	off;

	prop = dtb_node_find_prop(node, "compatible");
	if (DTB_IS_SENTINEL(prop))
		return FALSE;

	for (off = 0; off < prop.length;) {
		if (!strcmp(model, prop.data + off))
			return TRUE;
		off += strlen(prop.data + off) + 1;
	}

	return FALSE;
}

extern uint64_t dtb_prop_read_cells(
	const struct dtb_prop	*prop,
	unsigned short		size,
	vm_size_t		off)
{
	const void	*addr;
	uint64_t	tmp;

	assert(off + size <= prop->length);
	addr = (unsigned char *) prop->data + off;

	switch (size) {
		case 0:
			return 0;
		case 1:
			return be32toh(*(const dtb_uint32_t *) addr);
		case 2:
			__builtin_memcpy(&tmp, addr, 8);
			return __builtin_bswap64(tmp);
		default:
			panic("Unimplemented cell size: %d\n", size);
	}
}
