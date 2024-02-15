#ifndef _AARCH64_CPU_NUMBER_H_
#define _AARCH64_CPU_NUMBER_H_

#if NCPUS > 1

#error "TODO"

#else

#define CPU_NUMBER(addr)
#define CX(addr, reg)		addr

#endif

#endif /* _AARCH64_CPU_NUMBER_H_ */
