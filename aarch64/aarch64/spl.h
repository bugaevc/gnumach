#ifndef	_MACHINE_SPL_H_
#define	_MACHINE_SPL_H_

/*
 *	This file defines the interrupt priority levels used by
 *	machine-dependent code.
 */

typedef int		spl_t;

#define SPL0		0
#define SPL7		7

/* Enable interrupts.  */
static inline void spl0(void)
{
	__atomic_signal_fence(__ATOMIC_RELEASE);
	asm volatile("msr DAIFClr, #7");
}

/* Disable interrupts, returning previous SPL.  */
static inline spl_t spl7(void)
{
	long	daif;

	asm volatile("mrs %0, DAIF" : "=r"(daif));
	asm volatile("msr DAIFSet, #7");
	__atomic_signal_fence(__ATOMIC_ACQUIRE);

	/*
	 *	0x3c0 is SPSR_DAIF, but we'd rather avoid
	 *	including "aarch64/bits/spsr.h" into this
	 *	widely-used header.
	 */
	return (daif & 0x3c0) ? SPL7 : SPL0;
}

static inline void splx(spl_t spl)
{
	if (spl == SPL0)
		spl0();
}

static inline void spl0_irq(void)
{
	__atomic_signal_fence(__ATOMIC_RELEASE);
}

static inline void spl7_irq(void)
{
	__atomic_signal_fence(__ATOMIC_ACQUIRE);
}

#define splhigh		spl7
#define splsoftclock	spl7
#define splnet		spl7
#define splhdw		spl7
#define splbio		spl7
#define spldcm		spl7
#define spltty		spl7
#define splimp		spl7
#define splvm		spl7
#define splclock	spl7
#define splsched	spl7

#define spl1		spl7
#define spl2		spl7
#define spl3		spl7
#define spl4		spl7
#define spl5		spl7
#define spl6		spl7

#define assert_splsched() assert(splsched() == SPL7)

extern int spl_init;

static inline void setsoftclock(void)
{
	__builtin_unreachable();
}

#endif	/* _MACHINE_SPL_H_ */
