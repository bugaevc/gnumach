#include <device/dtb.h>

struct irq_ctlr;

extern void cnt_init(dtb_node_t node);

extern void cnt_set_interrupt_parent(
	dtb_node_t	node,
	struct irq_ctlr	*ctlr);

extern void startrtclock(void);
