#include "aarch64/percpu.h"

struct percpu percpu_array[NCPUS];

void init_percpu(int cpu)
{
	// memset(&percpu_array[cpu], 0, sizeof(struct percpu));
	asm volatile("msr TPIDRRO_EL0, %0" :: "r"(cpu));
	asm volatile("msr TPIDR_EL1, %0" :: "r"(&percpu_array[cpu]));
}
