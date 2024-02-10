#include <machine/task.h>

void machine_task_module_init (void)
{
}

void machine_task_init (task_t)
{
  /* FIXME: inherit from parent? */
}

void machine_task_terminate (task_t)
{
  /* Nothing to do here */
}

void machine_task_collect (task_t)
{
  /* Nor here */
}
