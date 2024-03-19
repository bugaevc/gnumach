#ifndef _AARCH64_CPU_NUMBER_H_
#define _AARCH64_CPU_NUMBER_H_

#if NCPUS > 1

#ifndef __ASSEMBLER__
static inline int cpu_number(void)
{
	int		mycpu;

	asm("mrs %0, TPIDRRO_EL0" : "=r"(mycpu));
	return mycpu;
}
#endif

#endif

#endif /* _AARCH64_CPU_NUMBER_H_ */
