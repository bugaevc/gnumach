#include <syscalls.h>
#include <testlib.h>
#include <mach/exception.h>
#include <mach.user.h>
#include <mach_port.user.h>

/*
 *	We'll make the thread itself run this function when it faults.
 *	This simulates handling a Unix SIGSEGV.
 */
static void __attribute__((noreturn)) fault_handler(
	vm_offset_t			fault_address,
	struct aarch64_thread_state	*ats)
{
	kern_return_t	kr;
	vm_size_t	i;
	vm_offset_t	addr = fault_address;

	printf("Handling a fault at %#016.llx, pc = %#016.llx\n", fault_address, ats->pc);

	/*
	 *	Allocate the missing area of memory.
	 */
	kr = vm_allocate(mach_task_self(), &addr, vm_page_size, FALSE);
	ASSERT_RET(kr, "failed to allocate missing memory");

	/*
	 *	Fill it with some data.
	 */
	for (i = 0; i < vm_page_size / sizeof(int); i++)
		*((int *) addr + i) = i;

	/*
	 *	Return back to the interrupted code.
	 */
	kr = thread_set_self_state(AARCH64_THREAD_STATE, (thread_state_t) ats,
				   AARCH64_THREAD_STATE_COUNT);
	ASSERT_RET(kr, "thread_set_self_state failed");
	FAILURE("thread_set_self_state returned");
}

/*
 *	The exception_raise() RPC handler.  Mach calls this when the other thread faults.
 *	This runs in a different thread; it could fix things up directly and resume the
 *	thread.  Instead, it sets things up so that the thread itself will fix things
 *	for itself, and then return back to what it was doing.
 */
kern_return_t catch_exception_raise(
	mach_port_t	exception_port,
	thread_t	thread,
	task_t		task,
	integer_t	exception,
	integer_t	code,
	long_integer_t	subcode)
{
	kern_return_t			kr;
	struct aarch64_thread_state	ats;
	mach_msg_type_number_t		ats_count = AARCH64_THREAD_STATE_COUNT;
	vm_offset_t			off;


	printf("Received exception_raise(%u %u %#016.llx)\n", exception, code, subcode);

	/*
	 *	We only want to handle EXC_BAD_ACCESS/KERN_INVALID_ADDRESS.
	 *	Return an error to proceed with the default handling otherwise.
	 */
	if (exception != EXC_BAD_ACCESS)
		return KERN_FAILURE;
	if (code != KERN_INVALID_ADDRESS)
		return KERN_FAILURE;

	kr = thread_get_state(thread, AARCH64_THREAD_STATE,
			      (thread_state_t) &ats, &ats_count);
	ASSERT_RET(kr, "thread_get_state get failed");
	ASSERT(ats_count == AARCH64_THREAD_STATE_COUNT, "bad ats_count");

	/*
	 *	Place a copy of the state on the thread's stack.
	 */
	off = ats.sp - sizeof(struct aarch64_thread_state);
	memcpy((void *) off, &ats, sizeof(struct aarch64_thread_state));

	/*
	 *	Make it call fault_handler(subcode, off).
	 */
	ats.sp = off;
	ats.pc = (vm_offset_t) fault_handler;
	ats.x[0] = (vm_offset_t) subcode;
	ats.x[1] = (vm_offset_t) off;

	kr = thread_set_state(thread, AARCH64_THREAD_STATE,
			      (thread_state_t) &ats, ats_count);
	ASSERT_RET(kr, "thread_set_state failed");

	/*
	 *	Our job here is done!  Returning success resumes the thread.
	 */
	mach_port_deallocate(mach_task_self(), thread);
	mach_port_deallocate(mach_task_self(), task);

	return KERN_SUCCESS;
}

static void exc_server_thread_body(void *arg)
{
	kern_return_t	kr;
	mach_port_t	exc_port = (mach_port_t) (vm_offset_t) arg;

	boolean_t exc_server(
		mach_msg_header_t	*request,
		mach_msg_header_t	*reply);

	kr = mach_msg_server(exc_server, 4096, exc_port, MACH_MSG_OPTION_NONE);
	ASSERT_RET(kr, "error in mach_msg_server");
}

static void do_count(void)
{
	const int	*arr = (const int *) 0x100000000;
	int		i;
	unsigned long	count = 0;

	for (i = 0; i < vm_page_size / sizeof(int) * 3; i++)
		count += arr[i];

	ASSERT(vm_page_size == 4096, "need a different answer for a different page size");
	ASSERT(count == 0x17fa00, "bad count");
}

int main(int argc, char *argv[], int envc, char *envp[])
{
	kern_return_t	kr;
	mach_port_t	exc_port = mach_reply_port();

	test_thread_start(mach_task_self(), exc_server_thread_body,
			  (void *) (vm_offset_t) exc_port);

	kr = mach_port_insert_right(mach_task_self(), exc_port,
				    exc_port, MACH_MSG_TYPE_MAKE_SEND);
	ASSERT_RET(kr, "mach_port_insert_right");

	kr = thread_set_exception_port(mach_thread_self(), exc_port);
	ASSERT_RET(kr, "thread_set_exception_port failed");

	do_count();

	return 0;
}
