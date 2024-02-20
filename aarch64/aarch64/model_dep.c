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
#include "aarch64/locore.h"
#include <device/cons.h>
#include <mach/machine.h>
#include <kern/printf.h>
#include <kern/startup.h>

char *kernel_cmdline;
char /*struct start_info*/ boot_info;
char irqtab;
char iunit;
char ivect;

int spl_init;

static void nommu_putc(char c)
{
	volatile char *out = (char*)0x09000000;
	*out = c;
}

void putc(char c)
{
 	volatile char *out = (char*)phystokv(0x09000000);
	*out = c;
}

void machine_idle(int mycpu)
{
#ifdef MACH_HYP
	hyp_idle();
#else
	// assert(cpu == cpu_number());
	asm volatile("wfi");
#endif
}

static vm_offset_t heap_start = 0x50000000;
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
	spl_init = TRUE;
}

static void zero_out_bss(void)
{
	extern void	__bss_start, __bss_end;

	memset(&__bss_start, 0, (char *) &__bss_end - (char *) &__bss_start);
}

void __attribute__((noreturn)) c_boot_entry(void)
{
	const char		*c;

	extern const char	version[];

	zero_out_bss();

	/*
	 *	Before we do anything else, print the hello message.
	 */
	for (c = version; *c; c++)
		nommu_putc(*c);
	nommu_putc('\n');

	/* FIXME This is specific to -machine virt -m 1G.  */
	vm_page_load(VM_PAGE_SEG_DMA, 0x40000000, 0x80000000);
	pmap_bootstrap();
	/* Now running with MMU from highmem, re-load things.  */
	asm volatile("" ::: "memory");
	pmap_bootstrap_misc();
	vm_page_load_heap(VM_PAGE_SEG_DMA, heap_start, 0x80000000);

	load_exception_vector_table();

	romputc = putc;

	machine_slot[0].is_cpu = TRUE;

	kernel_cmdline = "";

	setup_main();
	__builtin_unreachable();
}
