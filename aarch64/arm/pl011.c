#include "arm/pl011.h"
#include <device/dtb.h>

/*
 *	https://developer.arm.com/documentation/ddi0183/g/programmers-model/summary-of-registers
 *
 *	The following are offsets of the memory-mapped registers
 *	relative to a base address that is discoverable via the
 *	device tree.
 */

#define UARTDR		0x000
#define UARTRSR		0x004
#define UARTFR		0x018
#define UARTILPR	0x020
#define UARTIBRD	0x024
#define UARTFBRD	0x028
#define UARTLCR_H	0x02C
#define UARTCR		0x030
#define UARTIFLS	0x034
#define UARTIMSC	0x038
#define UARTRIS		0x03C
#define UARTMIS		0x040
#define UARTICR		0x044
#define UARTDMACR	0x048

/* TODO: support more than one instance */

static volatile unsigned char *base_address;
#define UART_REG(off, tp)	*(volatile tp *) (base_address + off)

void pl011_init(const struct dtb_node *node)
{
	struct dtb_prop		prop;
	uint64_t		addr;

	assert(dtb_node_is_compatible(node, "arm,pl011"));

	prop = dtb_node_find_prop(node, "reg");
	assert(!DTB_IS_SENTINEL(prop));

	addr = dtb_prop_read_cells(&prop, node->address_cells, 0);
	base_address = (volatile unsigned char *) phystokv(addr);
}

void pl011_putc(char c)
{
	UART_REG(UARTDR, char) = c;
}
