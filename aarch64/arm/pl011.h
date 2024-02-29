struct dtb_node;

void pl011_init(const struct dtb_node *node);

void pl011_putc(char c);
