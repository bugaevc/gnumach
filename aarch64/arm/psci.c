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

#include "arm/psci.h"
#include <device/dtb.h>
#include <kern/debug.h>
#include <string.h>

#define PSCI_VERSION		0x84000000
#define CPU_SUSPEND		0xc4000001
#define CPU_OFF			0x84000002
#define CPU_ON			0xc4000003
/* ...MIGRATE stuff... */
#define SYSTEM_OFF		0x84000008
#define SYSTEM_RESET		0x84000009
#define SYSTEM_RESET2		0c40000012
#define PSCI_FEATURES		0x8400000a
#define CPU_FREEZE		0x8400000b
#define CPU_DEFAULT_SUSPEND	0xc400000c
#define SYSTEM_SUSPEND		0xc400000e

static uint32_t cpu_off_id = CPU_OFF;

static enum {
	PSCI_METHOD_NONE,
	PSCI_METHOD_HVC,
	PSCI_METHOD_SMC
} psci_method = PSCI_METHOD_NONE;

static int smc32(
	uint32_t	function,
	uint32_t	arg1,
	uint32_t	arg2,
	uint32_t	arg3)
{
	register uint32_t w0 asm("w0") = function;
	register uint32_t w1 asm("w1") = arg1;
	register uint32_t w2 asm("w2") = arg2;
	register uint32_t w3 asm("w3") = arg3;

	asm volatile("smc #0"
		: "+r"(w0), "+r"(w1), "+r"(w2), "+r"(w3));

	return w0;
}

static int hvc32(
	uint32_t	function,
	uint32_t	arg1,
	uint32_t	arg2,
	uint32_t	arg3)
{
	register uint32_t w0 asm("w0") = function;
	register uint32_t w1 asm("w1") = arg1;
	register uint32_t w2 asm("w2") = arg2;
	register uint32_t w3 asm("w3") = arg3;

	asm volatile("hvc #0"
		: "+r"(w0), "+r"(w1), "+r"(w2), "+r"(w3));

	return w0;
}

kern_return_t psci_cpu_off(void)
{
	switch (psci_method) {
		case PSCI_METHOD_NONE:
			/* Not a chance.  */
			return KERN_FAILURE;
		case PSCI_METHOD_SMC:
			smc32(cpu_off_id, 0, 0, 0);
			return KERN_FAILURE;
		case PSCI_METHOD_HVC:
			hvc32(cpu_off_id, 0, 0, 0);
			return KERN_FAILURE;
		default:
			panic("Bad PSCI method\n");
	}
}

kern_return_t psci_system_off(void)
{
	switch (psci_method) {
		case PSCI_METHOD_NONE:
			/* Not a chance.  */
			return KERN_FAILURE;
		case PSCI_METHOD_SMC:
			smc32(SYSTEM_OFF, 0, 0, 0);
			return KERN_FAILURE;
		case PSCI_METHOD_HVC:
			hvc32(SYSTEM_OFF, 0, 0, 0);
			return KERN_FAILURE;
		default:
			panic("Bad PSCI method\n");
	}
}

kern_return_t psci_system_reset(void)
{
	switch (psci_method) {
		case PSCI_METHOD_NONE:
			/* Not a chance.  */
			return KERN_FAILURE;
		case PSCI_METHOD_SMC:
			smc32(SYSTEM_RESET, 0, 0, 0);
			return KERN_FAILURE;
		case PSCI_METHOD_HVC:
			hvc32(SYSTEM_RESET, 0, 0, 0);
			return KERN_FAILURE;
		default:
			panic("Bad PSCI method\n");
	}
}

void psci_init(dtb_node_t node)
{
	struct dtb_prop	prop;
	vm_size_t	off;

	dtb_for_each_prop (*node, prop) {
		if (!strcmp(prop.name, "method")) {
			if (!strcmp(prop.data, "smc")) {
				psci_method = PSCI_METHOD_SMC;
			} else if (!strcmp(prop.data, "hvc")) {
				psci_method = PSCI_METHOD_HVC;
			} else {
				panic("Unexpected PSCI method %s\n", (const char *) prop.data);
			}
		} else if (!strcmp(prop.name, "cpu_off")) {
			off = 0;
			cpu_off_id = dtb_prop_read_cells(&prop, 1, &off);
		}
		/* could add cpu_on etc here */
	}

	assert(psci_method != PSCI_METHOD_NONE);
}
