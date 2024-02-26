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
#include <kern/bootstrap.h>
#include <kern/boot_script.h>
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



const char *kernel_cmdline;
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

void halt_all_cpus(boolean_t reboot)
{
	/* TODO halt _all_ CPUs. */
	(void) reboot;
	printf("Shutdown completed successfully, now in tight loop.\n");
	printf("You can safely power off the system or hit ctl-alt-del to reboot\n");
	halt_cpu();
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

static void print_model(const char *model)
{
	const char	*c;
	const char	*message = "Model name: ";
	boolean_t	seen_comma = FALSE;

	for (c = message; *c; c++)
		nommu_putc(*c);
	for (c = model; *c; c++) {
		if (!seen_comma && *c == ',') {
			nommu_putc(' ');
			seen_comma = TRUE;
		} else {
			nommu_putc(*c);
		}
	}
	nommu_putc('\n');
}

static void initial_dtb_walk(void)
{
	struct dtb_node	node;
	struct dtb_prop	prop;

	node = dtb_root_node();
	/*
	 *	Look at root node's properties.
	 */
	dtb_for_each_prop (node, prop) {
		if (!strcmp(prop.name, "model")) {
			print_model(prop.data);
		}
	}

	/*
	 *	Look at top-level nodes and their props.
	 */
	dtb_for_each_child (node, node) {
		if (!strcmp(node.name, "chosen")) {
			prop = dtb_node_find_prop(&node, "bootargs");
			if (!DTB_IS_SENTINEL(prop))
				kernel_cmdline = (const char *) phystokv(prop.data);
			continue;
		}
		dtb_for_each_prop (node, prop) {
			if (!strcmp(prop.name, "device_type")) {
				if (!strcmp(prop.data, "memory"))
					nommu_putc('!');
			}
		}
	}
}

void __attribute__((noreturn)) c_boot_entry(dtb_t dtb)
{
	kern_return_t		kr;
	const char		*c;

	extern const char	version[];

	zero_out_bss();

	/*
	 *	Before we do anything else, print the hello message.
	 */
	for (c = version; *c; c++)
		nommu_putc(*c);
	nommu_putc('\n');

	kr = dtb_load(dtb);
	assert(kr == KERN_SUCCESS);

	initial_dtb_walk();

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

	if (kernel_cmdline == NULL)
		kernel_cmdline = "";
	printf("Kernel command line: %s\n", kernel_cmdline);

	/* Load dtb again, at the new address.  */
	kr = dtb_load((dtb_t) phystokv(dtb));
	assert(kr == KERN_SUCCESS);

	setup_main();
	__builtin_unreachable();
}

void machine_exec_boot_script(void)
{
	struct dtb_node 	chosen, node;
	struct dtb_prop 	prop;
	struct bootstrap_module	bmod;
	int			i = 0, err, losers = 0;
	const char		*args;

	chosen = dtb_node_by_path("/chosen");
	if (DTB_IS_SENTINEL(chosen))
		panic("No chosen node in DTB\n");

	dtb_for_each_child (chosen, node) {
		if (dtb_node_is_compatible(&node, "multiboot,module")) {
			prop = dtb_node_find_prop(&node, "bootargs");
			if (DTB_IS_SENTINEL(prop))
				panic("No bootargs for bootstrap module %d %s\n", i, node.name);
			args = (const char *) prop.data;
			printf("module %d: %s\n", i, args);
			prop = dtb_node_find_prop(&node, "reg");
			printf("reg:");
			for (int i = 0; i < prop.length; i++)
				printf(" %#02.x", *((const unsigned char *) prop.data + i));
			printf("\n");

			/* FIXME need a proper helper to read reg/cells */
			bmod.mod_start = __builtin_bswap64(*(const vm_offset_t *) prop.data);
			bmod.mod_end = bmod.mod_start + __builtin_bswap64(*((const vm_size_t *) prop.data + 1));

			/* FIXME: we probably should make a copy of this string */
			/* FIXME cannot pass on-stack bmod, it keeps the pointer */
			err = boot_script_parse_line(&bmod, args);
			if (err) {
				printf("Error: %s\n", boot_script_error_string(err));
				losers++;
			}
			i++;
		}
	}
	if (i == 0)
		panic("No bootstrap modules loaded with Mach\n");
	if (losers)
		panic("Failed to parse boot script\n");
	printf("%d bootstrap modules\n", i);
	err = boot_script_exec();
	if (err)
		panic("Failed to execute boot script: %s\n", boot_script_error_string(err));
	/* TODO free memory */
}
