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

#ifndef _AARCH64_SMP_H_
#define _AARCH64_SMP_H_

int smp_init(void);
void smp_remote_ast(unsigned apic_id);
void smp_pmap_update(unsigned apic_id);
void smp_startup_cpu(unsigned apic_id, unsigned vector);

#define cpu_pause()	asm volatile("yield" ::: "memory");

#endif /* _AARCH64_SMP_H_ */
