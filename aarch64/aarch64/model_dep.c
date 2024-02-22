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
#include "aarch64/hwcaps.h"
#include <device/cons.h>
#include <device/dtb.h>
#include <mach/machine.h>
#include <kern/printf.h>
#include <kern/startup.h>
#include <string.h>

/* Some ELF definitions, for applying relocations.  */

#define R_AARCH64_NONE		0
#define R_AARCH64_RELATIVE	1027

typedef uint64_t	Elf64_Addr;
typedef uint64_t	Elf64_Xword;
typedef int64_t		Elf64_Sxword;

typedef struct
{
	Elf64_Addr	r_offset;
	Elf64_Xword	r_info;
	Elf64_Sxword	r_addend;
} Elf64_Rela;



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

void halt_cpu(void)
{
#ifdef MACH_HYP
	hyp_halt();
#else
	/* Disable interrupts and WFE forever.  */
	asm volatile(
		"msr	DAIFClr, #2\n"
		"0:\n\t"
		"wfe\n\t"
		"b	0b"
	);
	__builtin_unreachable();
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
	hwcaps_init();

	spl_init = TRUE;
}

static void zero_out_bss(void)
{
	extern void	__bss_start, __bss_end;

	memset(&__bss_start, 0, (char *) &__bss_end - (char *) &__bss_start);
}

static void apply_runtime_relocations(void)
{
	extern const Elf64_Rela	__rela_start, __rela_end;
	extern const void	__text_start;

	const Elf64_Rela	*rela;
	Elf64_Addr		*addr;
	Elf64_Addr		slide;

	/* TODO: This assumes we're linked at base address 0x0.  */
	slide = (Elf64_Addr) &__text_start;

	for (rela = &__rela_start; rela != &__rela_end; rela++) {
		switch (rela->r_info) {
			case R_AARCH64_NONE:
				/* Nothing to do.  */
				break;
			case R_AARCH64_RELATIVE:
				addr = (Elf64_Addr *)(slide + rela->r_offset);
				*addr = slide + rela->r_addend;
				break;
			default:
				panic("Unimplemented relocation type\n");
		}
	}
}

void __attribute__((noreturn)) c_boot_entry(dtb_t dtb)
{
	kern_return_t		kr;
	const char		*c;

	extern const char	version[];

	zero_out_bss();

	kr = dtb_load(dtb);
	assert(kr == KERN_SUCCESS);

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
	apply_runtime_relocations();
	pmap_bootstrap_misc();
	vm_page_load_heap(VM_PAGE_SEG_DMA, heap_start, 0x80000000);

	load_exception_vector_table();

	romputc = putc;

	machine_slot[0].is_cpu = TRUE;

	kernel_cmdline = "";

	setup_main();
	__builtin_unreachable();
}
