/*
 * Copyright (c) 2024 Free Software Foundation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _AARCH64_TASK_
#define _AARCH64_TASK_

#include <kern/kern_types.h>

struct aarch64_pac_keys
{
  /* FIXME: are PAC keys even 64-bit? */
  uint64_t ia;
  uint64_t ib;
  uint64_t da;
  uint64_t db;
};

/* The machine specific data of a task.  */
struct machine_task
{
  struct aarch64_pac_keys apk;
};
typedef struct machine_task machine_task_t;

/* Initialize the machine task module.  The function is called once at
   start up by task_init in kern/task.c.  */
void machine_task_module_init (void);

/* Initialize the machine specific part of task TASK.  */
void machine_task_init (task_t);

/* Destroy the machine specific part of task TASK and release all
   associated resources.  */
void machine_task_terminate (task_t);

/* Try to release as much memory from the machine specific data in
   task TASK. */
void machine_task_collect (task_t);

#endif /* _AARCH64_TASK_ */
