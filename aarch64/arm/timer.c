#include "arm/timer.h"
#include <kern/assert.h>
#include "aarch64/mach_param.h" /* HZ */
#include <kern/mach_clock.h>

static unsigned cnt_freq;			/* frequency, timer ticks per second */
static volatile unsigned long long cnt_pct;

static void set_up_next_interrupt(void)
{
	asm volatile(
		"msr	CNTP_CVAL_EL0, %0"
		:: "r"(cnt_pct + cnt_freq / HZ));
}

void startrtclock(void)
{
	asm(
		"mrs	%0, CNTFRQ_EL0\n\t"
		"mrs	%1, CNTPCT_EL0"
		: "=r"(cnt_freq), "=r"(cnt_pct));
	assert(cnt_freq > 10);

	asm volatile(
		"msr	CNTP_CTL_EL0, %0"
		:: "r"(1)
	);

	set_up_next_interrupt();
}

void cnt_clock_interrupt(
	boolean_t	usermode,
	vm_offset_t	pc)
{
	unsigned long	last_pct = cnt_pct;
	unsigned int	usec;

	asm volatile(
		"isb\n\t"
		"mrs	%0, CNTPCT_EL0"
		: "=r"(cnt_pct));
	usec = (cnt_pct - last_pct) * 1000000 / cnt_freq;
	clock_interrupt(usec, usermode, FALSE /*??*/, pc);
	set_up_next_interrupt();
}
