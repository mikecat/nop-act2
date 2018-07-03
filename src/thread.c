#include "thread.h"
#include "thread_switch.h"
#include "memory.h"
#include "timer.h"

#define THREAD_MAX 8192

#define THREAD_YIELD_TIMER_INTERVAL 10

enum thread_status_t {
	THREAD_INVALID = 0,
	THREAD_VALID,
	THREAD_FINISHED
};

struct thread_data_t {
	enum thread_status_t thread_status;
	struct cpu_status_t cpu_status;
	void* stack_addr;
};

static struct thread_data_t threads[THREAD_MAX];
static int thread_head, thread_prev[THREAD_MAX], thread_next[THREAD_MAX];
static int thread_pool_head, thread_pool_next[THREAD_MAX];

static int current_thread;

static void thread_yield_timer(void* dummy) {
	timer_set(THREAD_YIELD_TIMER_INTERVAL, thread_yield_timer, 0);
	thread_yield();
	(void)dummy;
}

void thread_init(void) {
	int i;
	current_thread = -1;
	thread_head = -1;
	thread_pool_head = 0;
	for (i = 0; i < THREAD_MAX; i++) {
		thread_pool_next[i] = i + 1;
		threads[i].thread_status = THREAD_INVALID;
	}
	thread_pool_next[THREAD_MAX - 1] = -1;
	timer_set(THREAD_YIELD_TIMER_INTERVAL, thread_yield_timer, 0);
}

static void thread_create_internal(void (*thread_entry)(void*), void* thread_arg) {
	thread_entry(thread_arg);
	thread_exit();
}

int thread_create(void (*thread_entry)(void*), void* thread_arg, unsigned int stack_size) {
	void* stack;
	int eflags;
	int thread_id;
	if (thread_entry == 0 || stack_size < 12) return -1;
	if ((stack = memory_allocate(stack_size)) == 0) return -1;

	/* disable interrupt */
	__asm__ __volatile__(
		"pushf\n\t"
		"cli\n\t"
		"pop %%eax\n\t"
		"mov %%eax, %0\n\t"
	: "=m"(eflags) : : "%eax");

	thread_id = thread_pool_head;
	if (thread_id >= 0) thread_pool_head = thread_pool_next[thread_pool_head];

	/* enable interrupt only if previously enabled */
	__asm__ __volatile__(
		"testl $0x0200, %0\n\t"
		"jz 1f\n\t"
		"sti\n\t"
		"1:\n\t"
	: : "m"(eflags));

	if (thread_id < 0) {
		memory_free(stack);
		return -1;
	}

	threads[thread_id].cpu_status.eax = 0;
	threads[thread_id].cpu_status.ecx = 0;
	threads[thread_id].cpu_status.edx = 0;
	threads[thread_id].cpu_status.ebx = 0;
	threads[thread_id].cpu_status.esi = 0;
	threads[thread_id].cpu_status.edi = 0;
	threads[thread_id].cpu_status.esp = ((unsigned int)stack + stack_size - 12) & 0xfffffffcu;
	threads[thread_id].cpu_status.ebp = 0;
	threads[thread_id].cpu_status.eip = (unsigned int)thread_create_internal;
	threads[thread_id].cpu_status.eflags = 0x0202;
	threads[thread_id].cpu_status.cs = 0x08;
	threads[thread_id].cpu_status.ds = 0x10;
	threads[thread_id].cpu_status.es = 0x10;
	threads[thread_id].cpu_status.ss = 0x10;
	threads[thread_id].cpu_status.fs = 0x10;
	threads[thread_id].cpu_status.gs = 0x10;

	((unsigned int*)threads[thread_id].cpu_status.esp)[0] = 0;
	((unsigned int*)threads[thread_id].cpu_status.esp)[1] = (unsigned int)thread_entry;
	((unsigned int*)threads[thread_id].cpu_status.esp)[2] = (unsigned int)thread_arg;

	threads[thread_id].stack_addr = stack;
	threads[thread_id].thread_status = THREAD_VALID;

	/* disable interrupt */
	__asm__ __volatile__(
		"pushf\n\t"
		"cli\n\t"
		"pop %%eax\n\t"
		"mov %%eax, %0\n\t"
	: "=m"(eflags) : : "%eax");

	thread_prev[thread_id] = -1;
	thread_next[thread_id] = thread_head;
	if (thread_head >= 0) thread_prev[thread_head] = thread_id;
	thread_head = thread_id;

	/* enable interrupt only if previously enabled */
	__asm__ __volatile__(
		"testl $0x0200, %0\n\t"
		"jz 1f\n\t"
		"sti\n\t"
		"1:\n\t"
	: : "m"(eflags));

	return thread_id;
}

void thread_yield(void) {
	struct cpu_status_t *current, *next;
	int next_id;
	int eflags;

	/* disable interrupt */
	__asm__ __volatile__(
		"pushf\n\t"
		"cli\n\t"
		"pop %%eax\n\t"
		"mov %%eax, %0\n\t"
	: "=m"(eflags) : : "%eax");

	current = (current_thread < 0) ? 0 : &threads[current_thread].cpu_status;
	next_id = (current_thread < 0 || thread_next[current_thread] < 0) ?
		thread_head : thread_next[current_thread];
	if (next_id < 0) return;
	next = &threads[next_id].cpu_status;

	current_thread = next_id;
	switch_thread(current, next);

	/* enable interrupt only if previously enabled */
	__asm__ __volatile__(
		"testl $0x0200, %0\n\t"
		"jz 1f\n\t"
		"sti\n\t"
		"1:\n\t"
	: : "m"(eflags));
}

void thread_join(int tid) {
	int eflags;
	if (tid < 0 || THREAD_MAX <= tid || threads[tid].thread_status == THREAD_INVALID) return;
	while (threads[tid].thread_status != THREAD_FINISHED) {
		thread_yield();
	}

	/* disable interrupt */
	__asm__ __volatile__(
		"pushf\n\t"
		"cli\n\t"
		"pop %%eax\n\t"
		"mov %%eax, %0\n\t"
	: "=m"(eflags) : : "%eax");

	if (threads[tid].thread_status == THREAD_FINISHED) {
		threads[tid].thread_status = THREAD_INVALID;
		memory_free(threads[tid].stack_addr);
		thread_pool_next[tid] = thread_pool_head;
		thread_pool_head = tid;
	}

	/* enable interrupt only if previously enabled */
	__asm__ __volatile__(
		"testl $0x0200, %0\n\t"
		"jz 1f\n\t"
		"sti\n\t"
		"1:\n\t"
	: : "m"(eflags));
}

void thread_exit(void) {
	if (current_thread < 0) return;

	/* disable interrupt */
	__asm__ __volatile__("cli\n\t");

	threads[current_thread].thread_status = THREAD_FINISHED;
	if (thread_prev[current_thread] < 0) {
		thread_head = thread_next[current_thread];
	} else {
		thread_next[thread_prev[current_thread]] = thread_next[current_thread];
	}
	if (thread_next[current_thread] >= 0) {
		thread_prev[thread_next[current_thread]] = thread_prev[current_thread];
	}
	thread_yield();

	/* won't come here */
	__asm__ __volatile(
		"sti\n\t"
		"1:\n\t"
		"hlt\n\t"
		"jmp 1b\n\t"
	);
}
