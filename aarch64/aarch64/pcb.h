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

#ifndef _AARCH64_PCB_
#define _AARCH64_PCB_

#include <kern/task.h>
#include <kern/thread.h>
#include <mach/exec/exec.h>

extern void pcb_module_init(void);

extern void pcb_init(task_t parent_task, thread_t thread);
extern void pcb_terminate(thread_t thread);
extern void pcb_collect(thread_t thread);

extern void load_context(thread_t t);
extern void Thread_continue(void);

extern vm_offset_t user_stack_low (vm_size_t stack_size);

extern kern_return_t thread_setstatus(
   thread_t        thread,
   int             flavor,
   thread_state_t  tstate,
   unsigned int    count);

extern kern_return_t thread_getstatus(
   thread_t        thread,
   int             flavor,
   thread_state_t  tstate,
   unsigned int    *count);

extern void thread_set_syscall_return(
   thread_t        thread,
   kern_return_t   retval);

extern vm_offset_t set_user_regs(
   vm_offset_t	stack_base,
   vm_offset_t	stack_size,
   const struct	exec_info *exec_info,
   vm_size_t	arg_size);

extern void stack_attach(
   thread_t	thread,
   vm_offset_t	stack,
   void		(*continuation)(thread_t));

extern vm_offset_t stack_detach(thread_t thread);

#endif /* _AARCH64_PCB_ */
