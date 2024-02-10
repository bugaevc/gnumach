#include "machine/trap.h"
#include <kern/thread.h>

#if     MACH_PCSAMPLE > 0
/*
 * return saved state for interrupted user thread
 */
unsigned
interrupted_pc(const thread_t t)
{
	return 0x1234;
}
#endif  /* MACH_PCSAMPLE > 0 */
