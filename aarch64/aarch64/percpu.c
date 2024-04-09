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

#include "aarch64/percpu.h"

struct percpu percpu_array[NCPUS];

void init_percpu(int cpu)
{
	// memset(&percpu_array[cpu], 0, sizeof(struct percpu));
	asm volatile("msr TPIDRRO_EL0, %0" :: "r"(cpu));
	asm volatile("msr TPIDR_EL1, %0" :: "r"(&percpu_array[cpu]));
}
