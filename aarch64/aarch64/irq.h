/*
 * Copyright (c) 2024 Free Software Foundation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _AARCH64_IRQ_
#define _AARCH64_IRQ_

#include <device/dtb.h>

/* Legacy intr cruft to make device/intr.h compile, do not actually use this */
#ifdef __INTR_H__

#define NINTR 1234

typedef unsigned int irq_t;

// FIXME move to inter.h
extern struct irqdev irqtab;

// FIXME does it make sense on aarch64?
extern int   iunit[];
typedef void (*interrupt_handler_fn)(int);
extern interrupt_handler_fn ivect[];

void intnull(int unit);

void __enable_irq (irq_t irq);
void __disable_irq (irq_t irq);

extern void unmask_irq (unsigned int irq_nr);

#endif /* device/intr.h legacy */


struct irq_src {
	void	(*handle_irq)(struct irq_src *);
};

typedef enum {
	IRQ_DESC_TYPE_DT = 0,
} irq_desc_type;

struct irq_desc {
	irq_desc_type	type;
};

struct irq_desc_dt {
	irq_desc_type	type;
	dtb_prop_t	prop;
};

struct irq_ctlr {
	void	(*add_src)(struct irq_ctlr		*ctlr,
			   struct irq_src		*src,
			   const struct irq_desc	*desc);
};

extern struct irq_src	*root_irq_src;

#endif /* _AARCH64_IRQ_ */
