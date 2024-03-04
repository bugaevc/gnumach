#include "arm/gic-v2.h"

#define GICD_CTLR		0x000
#define GICD_ISENABLER		0x100
#define GICD_ICENABLER		0x180
#define GICD_ISPENDR		0x200
#define GICD_ICPENDR		0x280

#define GICD_CTLR_DISABLE	0x0
#define GICD_CTLR_ENABLE	0x1

#define GICC_CTLR		0x000
#define GICC_PMR		0x004
#define GICC_BPR		0x008
#define GICC_IAR		0x00c
#define GICC_EOIR		0x010

#define GICC_CTLR_DISABLE	0x0
#define GICC_CTLR_ENABLE	0x1

static volatile unsigned char	*distributor_base;
static vm_size_t		distributor_size;
static volatile unsigned char	*cpu_base;
static vm_size_t		cpu_size;

#define GICD_REG(off)		*(volatile uint32_t *) (distributor_base + off)
#define GICC_REG(off)		*(volatile uint32_t *) (cpu_base + off)

void gic_v2_init(dtb_node_t node, dtb_ranges_map_t map)
{
	struct dtb_prop	prop;
	uint64_t	tmp;
	vm_offset_t	off = 0;

	prop = dtb_node_find_prop(node, "reg");
	assert(!DTB_IS_SENTINEL(prop));

	tmp = dtb_prop_read_cells(&prop, node->address_cells, &off);
	tmp = dtb_map_address(map, tmp);
	distributor_base = (volatile unsigned char *) phystokv(tmp);

	distributor_size = dtb_prop_read_cells(&prop, node->size_cells, &off);

	tmp = dtb_prop_read_cells(&prop, node->address_cells, &off);
	tmp = dtb_map_address(map, tmp);
	cpu_base = (volatile unsigned char *) phystokv(tmp);

	cpu_size = dtb_prop_read_cells(&prop, node->size_cells, &off);
}

void gic_v2_enable(void)
{
	GICD_REG(GICD_CTLR) = GICD_CTLR_ENABLE;
	GICC_REG(GICC_CTLR) = GICC_CTLR_ENABLE;
	GICC_REG(GICC_PMR) = 0xff;
	GICC_REG(GICC_BPR) = 0;
}

void gic_v2_enable_irq(int irq)
{
	GICD_REG(GICD_ISENABLER + (irq / 32)) = 1 << (irq % 32);
}

void gic_v2_disable_irq(int irq)
{
	GICD_REG(GICD_ICENABLER + (irq / 32)) = 1 << (irq % 32);
}

boolean_t gic_v2_check_irq(int irq) {
	return (GICD_REG(GICD_ICPENDR + (irq / 32)) & (1 << (irq % 32))) != 0;
}

void gic_v2_clear_irq(int irq)
{
	GICD_REG(GICD_ICPENDR + (irq / 32)) = 1 << (irq % 32);
}
