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

#ifndef _AARCH64_MODEL_DEP_H_
#define _AARCH64_MODEL_DEP_H_

#include <mach/std_types.h>
#include <device/dtb.h>

/*
 * Find devices.  The system is alive.
 */
extern void machine_init (void);

/* Conserve power on processor CPU.  */
extern void machine_idle (int cpu);

extern void resettodr (void);

extern void startrtclock (void);

/*
 * Halt a cpu.
 */
extern void halt_cpu (void) __attribute__ ((noreturn));

/*
 * Halt the system or reboot.
 */
extern void halt_all_cpus (boolean_t reboot) __attribute__ ((noreturn));

/*
 * Make cpu pause a bit.
 */
extern void machine_relax (void);

/*
 * C boot entrypoint - called by boot_entry in boothdr.S.
 */
extern void c_boot_entry(dtb_t dtb) __attribute__ ((noreturn));

#endif /* _AARCH64_MODEL_DEP_H_ */
