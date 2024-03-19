#include "arm/timer.h"
#include "aarch64/irq.h"
#include <kern/assert.h>
#include "aarch64/mach_param.h" /* HZ */
#include <kern/mach_clock.h>
#include <string.h>

static unsigned cnt_freq;			/* frequency, timer ticks per second */
static volatile unsigned long long cnt_pct;

static void set_up_next_interrupt(void)
{
	asm volatile(
		"msr	CNTP_CVAL_EL0, %0"
		:: "r"(cnt_pct + cnt_freq / HZ));
}

void startrtclock(void)
{
	asm(
		"mrs	%0, CNTFRQ_EL0\n\t"
		"mrs	%1, CNTPCT_EL0"
		: "=r"(cnt_freq), "=r"(cnt_pct));
	assert(cnt_freq > 10);

	asm volatile(
		"msr	CNTP_CTL_EL0, %0"
		:: "r"(1)
	);

	set_up_next_interrupt();
}

static void cnt_handle_irq(struct irq_src *)
{
	unsigned long	last_pct = cnt_pct;
	unsigned int	usec;
	boolean_t	from_el0;

	asm volatile(
		"isb\n\t"
		"mrs	%0, CNTPCT_EL0"
		: "=r"(cnt_pct));
	usec = (cnt_pct - last_pct) * 1000000 / cnt_freq;

	from_el0 = percpu_get(boolean_t, in_irq_from_el0);
	if (from_el0)
		clock_interrupt(usec, TRUE, TRUE, current_thread()->pcb->ats.pc);
	else
		clock_interrupt(usec, FALSE, TRUE, 0);

	set_up_next_interrupt();
}

void cnt_set_interrupt_parent(dtb_node_t node, struct irq_ctlr *ctlr)
{
	struct dtb_prop		prop;
	struct irq_desc_dt	desc;
	static struct irq_src	src;

	prop = dtb_node_find_prop(node, "interrupts");
	assert(!DTB_IS_SENTINEL(prop));

	desc.type = IRQ_DESC_TYPE_DT;
	desc.prop = &prop;

	src.handle_irq = cnt_handle_irq;
	ctlr->add_src(ctlr, &src, (struct irq_desc *) &desc);
}

void cnt_init(dtb_node_t node)
{
	struct dtb_prop	prop;

	asm("mrs %0, CNTFRQ_EL0" : "=r"(cnt_freq));

	dtb_for_each_prop (*node, prop) {
		if (!strcmp(prop.name, "clock-frequency")) {
			vm_offset_t	off = 0;
			cnt_freq = dtb_prop_read_cells(&prop, 1, &off);
		}
	}
}
