/*
 *  Copyright (C) 2022 Free Software Foundation
 *
 * This program is free software ; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation ; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY ; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the program ; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

        .section .text.boot
        .globl  _start
_start:
        ldr     x1, =_boot_stack_end
        mov     sp, x1

        // enable FPU for vector instructions... or should we disable them?
        mrs     x1, cpacr_el1
        mov     x0, #(3 << 20)
        orr     x0, x1, x0
        msr     cpacr_el1, x0

        b       c_boot_entry
        b       .       // in case we return

_boot_stack:
        .space  4096
_boot_stack_end:
