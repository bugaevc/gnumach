#include "i386/i386/multiboot.h"
#include <kern/bootstrap.h>
#include <kern/boot_script.h>

void machine_exec_boot_script(void)
{
  int losers, n, i;

  if (!(boot_info.flags & MULTIBOOT_MODS)
      || (boot_info.mods_count == 0))
    panic ("No bootstrap code loaded with the kernel!");


#ifdef	MACH_XEN
#ifdef __x86_64__ // 32_ON_64 actually
  struct multiboot32_module *bmods = (struct multiboot32_module *)
                                     boot_info.mod_start;
  if (bmods32) {
    int i;
    for (n = 0; bmods32[n].mod_start; n++)
      ;
    bmods = alloca(n * sizeof(*bmods));
    for (i = 0; i < n ; i++)
    {
      bmods[i].mod_start = kvtophys(bmods32[i].mod_start + (vm_offset_t) bmods32);
      bmods[i].mod_end = kvtophys(bmods32[i].mod_end + (vm_offset_t) bmods32);
      bmods[i].string = kvtophys(bmods32[i].string + (vm_offset_t) bmods32);
    }
  }
#else
  struct multiboot_module *bmods = (struct multiboot_module *)
                                   boot_info.mod_start;
  if (bmods)
    for (n = 0; bmods[n].mod_start; n++) {
      bmods[n].mod_start = kvtophys(bmods[n].mod_start + (vm_offset_t) bmods);
      bmods[n].mod_end = kvtophys(bmods[n].mod_end + (vm_offset_t) bmods);
      bmods[n].string = kvtophys(bmods[n].string + (vm_offset_t) bmods);
    }
#endif
  boot_info.mods_count = n;
  boot_info.flags |= MULTIBOOT_MODS;
#else	/* MACH_XEN */
#ifdef __x86_64__
  struct multiboot_raw_module *bmods32 = ((struct multiboot_raw_module *)
                                          phystokv(boot_info.mods_addr));
  struct multiboot_module *bmods=NULL;
  if (bmods32)
    {
      int i;
      bmods = alloca(boot_info.mods_count * sizeof(*bmods));
      for (i=0; i<boot_info.mods_count; i++)
        {
          bmods[i].mod_start = bmods32[i].mod_start;
          bmods[i].mod_end = bmods32[i].mod_end;
          bmods[i].string = bmods32[i].string;
        }
    }
#else
  struct multiboot_module *bmods = ((struct multiboot_module *)
				    phystokv(boot_info.mods_addr));
#endif
#endif	/* MACH_XEN */

  for (i = 0; i < boot_info.mods_count; ++i)
    {
      int err;
      char *line = (char*)phystokv(bmods[i].string);
      printf ("module %d: %s\n", i, line);
      err = boot_script_parse_line (&bmods[i], line);
      if (err)
	{
	  printf ("\n\tERROR: %s", boot_script_error_string (err));
	  ++losers;
	}
    }

  printf ("%d multiboot modules\n", i);
  if (losers)
    panic ("%d of %d boot script commands could not be parsed",
	   losers, boot_info.mods_count);
  losers = boot_script_exec ();
  if (losers)
    panic ("ERROR in executing boot script: %s",
	   boot_script_error_string (losers));

  /* XXX we could free the memory used
     by the boot loader's descriptors and such.  */
  for (n = 0; n < boot_info.mods_count; n++)
    free_bootstrap_pages(bmods[n].mod_start, bmods[n].mod_end);
}
