#include <kern/thread.h>

extern void fpu_init(void);
extern void fpu_switch_context(thread_t new);
extern void fpu_access_trap(void);
extern void fpu_free(thread_t thread);

extern void fpu_flush_state_read(thread_t thread);
extern void fpu_flush_state_write(thread_t thread);
