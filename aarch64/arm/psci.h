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

#include <device/dtb.h>

#define psci_is_compatible(node)					\
	(dtb_node_is_compatible(node, "arm,psci")			\
	|| dtb_node_is_compatible(node, "arm,psci-0.2")			\
	|| dtb_node_is_compatible(node, "arm,psci-1.0"))

extern void psci_init(dtb_node_t node);
extern kern_return_t psci_cpu_off(void);
extern kern_return_t psci_system_off(void);
extern kern_return_t psci_system_reset(void);
