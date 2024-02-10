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

#include "aarch64/model_dep.h"
#include <device/cons.h>
#include <mach/machine.h>
#include <kern/printf.h>
#include <kern/startup.h>

char *kernel_cmdline;

void putc(char c)
{
  volatile char *out = (char*)0x09000000;
  *out = c;
}

static vm_offset_t heap_start = 0x40000000;
vm_offset_t pmap_grab_page(void)
{
  vm_offset_t res = heap_start;
  heap_start += PAGE_SIZE;
  return res;
}

/*
 * Find devices.  The system is alive.
 */
void machine_init(void)
{
}

void c_boot_entry(void)
{
  romputc = putc;

  kernel_cmdline = "";

  /* Before we do anything else, print the hello message.  */
  extern const char version[];
  printf("%s\n", version);

  /* FIXME This is specific to -machine virt -m 1G.  */
  vm_page_load(VM_PAGE_SEG_DMA, 0x40000000, 0x80000000);
  pmap_bootstrap();
  vm_page_load_heap(VM_PAGE_SEG_DMA, heap_start, 0x80000000);

  machine_slot[0].is_cpu = TRUE;

  setup_main();
  __builtin_unreachable();
}
