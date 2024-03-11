#include "arm/gic-v2.h"
#include "aarch64/irq.h"
#include <kern/printf.h>
#include <string.h>

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

#define GICC_IAR_IRQ_MASK	0x7ff

#define GIC_IRQ_SPURIOUS	1023

struct gic_v2 {
	struct irq_src	src;
	struct irq_ctlr	ctlr;

	void		*distributor_base;
	vm_size_t	distributor_size;
	void		*cpu_base;
	vm_size_t	cpu_size;

	vm_size_t	nsrcs;
	struct irq_src	**srcs;
};

#define GICD_REG(gic, off)	*(volatile uint32_t *) ((gic)->distributor_base + (off))
#define GICC_REG(gic, off)	*(volatile uint32_t *) ((gic)->cpu_base + (off))

static void gic_v2_enable_irq(struct gic_v2 *gic, int irq)
{
	GICD_REG(gic, GICD_ISENABLER + (irq / 32)) = 1 << (irq % 32);
}

static void gic_v2_disable_irq(struct gic_v2 *gic, int irq)
{
	GICD_REG(gic, GICD_ICENABLER + (irq / 32)) = 1 << (irq % 32);
}

static void gic_v2_handle_irq(struct irq_src *data)
{
	struct gic_v2	*gic = structof(data, struct gic_v2, src);
	struct irq_src	*src;
	uint32_t	iar;
	uint32_t	irq;

	iar = GICC_REG(gic, GICC_IAR);
	irq = iar & GICC_IAR_IRQ_MASK;

	if (unlikely(irq == GIC_IRQ_SPURIOUS))
		return;

	if (irq < 16 || irq - 16 >= gic->nsrcs)
		goto BadIrq;
	src = gic->srcs[irq - 16];
	if (src == NULL)
		goto BadIrq;

	src->handle_irq(src);

	GICC_REG(gic, GICC_EOIR) = iar;

	/* TODO: loop perhaps? */

	return;

BadIrq:
	printf("GIC v2: unexpected IRQ %u\n", irq);
	gic_v2_disable_irq(gic, irq);
}

static void gic_v2_add_src(
	struct irq_ctlr		*ctlr,
	struct irq_src		*src,
	const struct irq_desc	*desc)
{
	struct gic_v2	*gic = structof(ctlr, struct gic_v2, ctlr);
	dtb_prop_t	prop;
	vm_size_t	off;

	assert(desc->type == IRQ_DESC_TYPE_DT);
	prop = ((const struct irq_desc_dt *) desc)->prop;
	assert(prop->length % 12 == 0);

	for (off = 0; off < prop->length;) {
		uint32_t	cell, flags;
		boolean_t	is_ppi;
		int		irq;

		cell = dtb_prop_read_cells(prop, 1, &off);
		is_ppi = (cell == 1);
		cell = dtb_prop_read_cells(prop, 1, &off);
		irq = cell + (is_ppi ? 16 : 32);
		cell = dtb_prop_read_cells(prop, 1, &off);
		flags = cell;

		if (irq - 16 >= gic->nsrcs) {
			struct irq_src	**srcs;

			srcs = (struct irq_src **) kalloc(sizeof(struct irq_src *) * (irq - 16 + 1));
			if (gic->nsrcs > 0)
				memcpy(srcs, gic->srcs, sizeof(struct irq_src *) * gic->nsrcs);
			memset(srcs + gic->nsrcs, sizeof(struct irq_src *) * (irq - 16 + 1 - gic->nsrcs), 0);
			if (gic->srcs != NULL)
				kfree((vm_offset_t) gic->srcs, sizeof(struct irq_src *) * gic->nsrcs);
			gic->srcs = srcs;
			gic->nsrcs = irq - 16 + 1;
		}
		assert(gic->srcs[irq - 16] == NULL);
		gic->srcs[irq - 16] = src;

		gic_v2_enable_irq(gic, irq);
		/* TODO: flags */
	}
}

struct irq_ctlr *gic_v2_init(dtb_node_t node, dtb_ranges_map_t map)
{
	struct gic_v2	*gic;
	struct dtb_prop	prop;
	uint64_t	tmp;
	vm_offset_t	off = 0;

	gic = (struct gic_v2 *) kalloc(sizeof(struct gic_v2));
	gic->nsrcs = 0;
	gic->srcs = NULL;
	gic->src.handle_irq = gic_v2_handle_irq;
	gic->ctlr.add_src = gic_v2_add_src;

	prop = dtb_node_find_prop(node, "reg");
	assert(!DTB_IS_SENTINEL(prop));

	tmp = dtb_prop_read_cells(&prop, node->address_cells, &off);
	tmp = dtb_map_address(map, tmp);
	gic->distributor_base = (void *) phystokv(tmp);

	gic->distributor_size = dtb_prop_read_cells(&prop, node->size_cells, &off);

	tmp = dtb_prop_read_cells(&prop, node->address_cells, &off);
	tmp = dtb_map_address(map, tmp);
	gic->cpu_base = (void *) phystokv(tmp);

	gic->cpu_size = dtb_prop_read_cells(&prop, node->size_cells, &off);

	/* TODO: support GICv2 not being the root controller */
	root_irq_src = &gic->src;

	return &gic->ctlr;
}

void gic_v2_enable(struct irq_ctlr *ctlr)
{
	struct gic_v2	*gic = structof(ctlr, struct gic_v2, ctlr);

	GICD_REG(gic, GICD_CTLR) = GICD_CTLR_ENABLE;
	GICC_REG(gic, GICC_CTLR) = GICC_CTLR_ENABLE;
	GICC_REG(gic, GICC_PMR) = 0xff;
	GICC_REG(gic, GICC_BPR) = 0;
}
