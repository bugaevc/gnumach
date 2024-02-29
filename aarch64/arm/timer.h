#include <mach/boolean.h>
#include <mach/machine/vm_types.h>

extern void startrtclock(void);
extern void cnt_clock_interrupt(
	boolean_t	usermode,
	vm_offset_t	pc);
