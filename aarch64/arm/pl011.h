#include <device/dtb.h>

void pl011_early_init(dtb_node_t node, dtb_ranges_map_t map);
void pl011_init(dtb_node_t node, dtb_ranges_map_t map);

void pl011_putc(char c);
