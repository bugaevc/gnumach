#include "arm/pl011.h"
#include <device/dtb.h>
#include <device/cons.h>
#include <device/io_req.h>
#include <device/ds_routines.h>	/* device_io_map */
#include <vm/vm_user.h>
#include "aarch64/smp.h"	/* cpu_pause() */
#include <string.h>

/*
 *	Mach driver for PrimeCell UART (PL011).
 *
 *	https://developer.arm.com/documentation/ddi0183/g/programmers-model/summary-of-registers
 *
 *	The following are offsets of the memory-mapped registers
 *	relative to a base address that is discoverable via the
 *	device tree.
 */

#define UARTDR			0x000	/* data */
#define UARTRSR			0x004	/* receive status / error clear */
					/* 0x008 to 0x014 are reserved */
#define UARTFR			0x018	/* flags */
					/* 0x01c is reserved */
#define UARTILPR		0x020	/* IrDA low-power counter */
#define UARTIBRD		0x024	/* integer baud rate */
#define UARTFBRD		0x028	/* fractional baud rate */
#define UARTLCR_H		0x02C	/* line control */
#define UARTCR			0x030	/* control */
#define UARTIFLS		0x034	/* interrupt FIFO level select */
#define UARTIMSC		0x038	/* interrupt mask set/clear */
#define UARTRIS			0x03C	/* raw interrupt status */
#define UARTMIS			0x040	/* masked interrupt status */
#define UARTICR			0x044	/* interrrupt clear */
#define UARTDMACR		0x048	/* DMA control */

#define UARTDR_DATA_MASK	0x000f
#define UARTDR_FE		0x0010	/* framing error */
#define UARTDR_PE		0x0020	/* parity error */
#define UARTDR_BE		0x0040	/* break error */
#define UARTDR_OE		0x0080	/* overrun error */

#define UARTFR_CTS		0x0001	/* clear to send */
#define UARTFR_DSR		0x0002	/* data set ready */
#define UARTFR_DCD		0x0004	/* data carrier detect */
#define UARTFR_BUSY		0x0008	/* busy */
#define UARTFR_RXFE		0x0010	/* receive FIFO empty */
#define UARTFR_TXFF		0x0020	/* transmit FIFO full */
#define UARTFR_RXFF		0x0040	/* receive FIFO full */
#define UARTFR_TXFE		0x0080	/* transmit FIFO empty */
#define UARTFR_RI		0x0100	/* ring indicator */

#define UARTLCR_H_BRK		0x0001	/* send break */
#define UARTLCR_H_PEN		0x0002	/* parity enable */
#define UARTLCR_H_EPS		0x0004	/* even parity select */
#define UARTLCR_H_STP2		0x0008	/* two stop bits select */
#define UARTLCR_H_FEN		0x0010	/* enable FIFOs */
#define UARTLCR_H_WLEN_5	0x0000	/* 5-bit words */
#define UARTLCR_H_WLEN_6	0x0020	/* 6-bit words */
#define UARTLCR_H_WLEN_7	0x0040	/* 7-bit words */
#define UARTLCR_H_WLEN_8	0x0060	/* 8-bit words */
#define UARTLCR_H_SPS		0x0080	/* stick parity select */

#define UARTCR_UARTEN		0x0001	/* UART enable */
#define UARTCR_SIREN		0x0002	/* SIR enable */
#define UARTCR_SIRLP		0x0004	/* SIR low-power mode */
					/* 6-3 reserved */
#define UARTCR_LBE		0x0080	/* loopback enable */
#define UARTCR_TXE		0x0100	/* transmit enable */
#define UARTCR_RXE		0x0200	/* receive enable */
#define UARTCR_DTR		0x0400	/* data transmit ready */
#define UARTCR_RTS		0x0800	/* request to send */
#define UARTCR_OUT1		0x1000	/* Out1 */
#define UARTCR_OUT2		0x2000	/* Out2 */
#define UARTCR_RTSEN		0x4000	/* RTS hardware flow control enable */
#define UARTCR_CTSEN		0x8000	/* CTS hardware flow control enable */

struct pl011 {
	struct mach_device	dev;
	void			*base;
};

#define UART_REG(uart, off, tp)		*(volatile tp *) ((uart)->base + (off))

static vm_offset_t	pl011_rom_base;
static void pl011_romputc(char c)
{
	while (unlikely((*(volatile uint32_t *) (pl011_rom_base + UARTFR)) & UARTFR_TXFF))
		cpu_pause();
	*(volatile uint32_t *) (pl011_rom_base + UARTDR) = c;
}

void pl011_early_init(dtb_node_t node, dtb_ranges_map_t map)
{
	struct dtb_prop	prop;
	vm_size_t	off;
	uint64_t	addr = 0x1;

	if (romputc != NULL)
		return;

	dtb_for_each_prop (*node, prop) {
		if (!strcmp(prop.name, "status")) {
			if (strncmp(prop.data, "ok", 2))
				return;
		} else if (!strcmp(prop.name, "reg")) {
			off = 0;
			addr = dtb_prop_read_cells(&prop, node->address_cells, &off);
			addr = dtb_map_address(map, addr);
		}
	}

	assert(addr != 0x1);
	pl011_rom_base = phystokv(addr);	/* FIXME can't call phystokv at this point */
	romputc = (void (*)(char)) phystokv(pl011_romputc);
}

/* FIXME more than one */
static struct pl011 the_uart;

void pl011_init(dtb_node_t node, dtb_ranges_map_t map)
{
	struct dtb_prop	prop;
	uint64_t	addr;
	vm_size_t	off = 0;

	assert(dtb_node_is_compatible(node, "arm,pl011"));

	prop = dtb_node_find_prop(node, "reg");
	assert(!DTB_IS_SENTINEL(prop));

	addr = dtb_prop_read_cells(&prop, node->address_cells, &off);
	addr = dtb_map_address(map, addr);
	the_uart.base = (void *) phystokv(addr);
}

static boolean_t pl011_txff(struct pl011 *uart)
{
	return !!(UART_REG(uart, UARTFR, uint32_t) & UARTFR_TXFF);
}

#if 0
void pl011_putc(char c)
{
	while (pl011_txff(&the_uart))
		cpu_pause();
	UART_REG(&the_uart, UARTDR, char) = c;
}
#endif

static io_return_t pl011_write(dev_t dev, io_req_t ior)
{
	io_return_t	kr;
	vm_offset_t	map_addr;
	const char	*data;
	struct pl011	*uart = structof(ior->io_device, struct pl011, dev);

	if (ior->io_total == 0)
		return D_SUCCESS;

	if (!(UART_REG(uart, UARTFR, uint32_t) & UARTFR_DCD))
		return D_IO_ERROR;

	if (ior->io_op & IO_INBAND) {
		data = ior->io_data;
	} else {
		kr = vm_map_copyout(device_io_map, &map_addr, (vm_map_copy_t) ior->io_data);
		if (kr != KERN_SUCCESS)
			return kr;
		data = (char *) map_addr;
	}

	/* TODO: don't block */
	ior->io_residual = ior->io_total;
	while (ior->io_residual > 0) {
		while (pl011_txff(uart))
			cpu_pause();
		UART_REG(uart, UARTDR, char) = *data;
		data++;
		ior->io_residual--;
	}

	if (!(ior->io_op & IO_INBAND)) {
		kr = vm_deallocate(device_io_map, map_addr, ior->io_count);
		assert(kr == KERN_SUCCESS);
	}

	return D_SUCCESS;
}
