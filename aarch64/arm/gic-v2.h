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

#include <mach/boolean.h>
#include <device/dtb.h>

#define gic_v2_is_compatible(node)					\
	(dtb_node_is_compatible(node, "arm,arm1176jzf-devchip-gic")	\
	|| dtb_node_is_compatible(node, "arm,arm11mp-gic")		\
	|| dtb_node_is_compatible(node, "arm,cortex-a15-gic")		\
	|| dtb_node_is_compatible(node, "arm,cortex-a7-gic")		\
	|| dtb_node_is_compatible(node, "arm,cortex-a9-gic")		\
	|| dtb_node_is_compatible(node, "arm,eb11mp-gic")		\
	|| dtb_node_is_compatible(node, "arm,gic-400")			\
	|| dtb_node_is_compatible(node, "arm,pl390")			\
	|| dtb_node_is_compatible(node, "arm,tc11mp-gic")		\
	|| dtb_node_is_compatible(node, "brcm,brahma-b15-gic")		\
	|| dtb_node_is_compatible(node, "nvidia,tegra210-agic")		\
	|| dtb_node_is_compatible(node, "qcom,msm-8660-qgic")		\
	|| dtb_node_is_compatible(node, "qcom,msm-qgic2"))

extern struct irq_ctlr *gic_v2_init(dtb_node_t node, dtb_ranges_map_t map);
extern void gic_v2_enable(struct irq_ctlr *);
