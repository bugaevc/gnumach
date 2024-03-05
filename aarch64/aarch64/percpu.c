#include "aarch64/percpu.h"

struct percpu percpu_array[NCPUS];

void init_percpu(int cpu)
{
	// memset(&percpu_array[cpu], 0, sizeof(struct percpu));
	percpu_array[cpu].self = &percpu_array[cpu];
}
