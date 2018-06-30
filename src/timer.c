#include "timer.h"
#include "io.h"
#include "memory.h"
#include "interrupts.h"

struct timer_data_t {
	struct timer_data_t* next;
	int timeout;
	int timeout_push;
	void (*callback)(void*);
	void* user_data;
};

static struct timer_data_t *timer_list, *timer_data_pool;

static void timer_interrupt_handler(int ecode) {
	struct timer_data_t* timeout_list = 0, *list_next;
	int push = 1;
	/* update timer list */
	while (timer_list != 0) {
		timer_list->timeout -= push;
		timer_list->timeout_push += push;
		if (timer_list->timeout > 0) break;
		/* timeout */
		push = timer_list->timeout_push;
		list_next = timer_list->next;
		timer_list->next = timeout_list;
		timeout_list = timer_list;
		timer_list = list_next;
	}
	/* call callback for timeout */
	while (timeout_list != 0) {
		/* get callback info */
		void (*callback)(void*) = timeout_list->callback;
		void* user_data = timeout_list->user_data;
		/* remove node from list */
		list_next = timeout_list->next;
		timeout_list->next = timer_data_pool;
		timer_data_pool = timeout_list;
		timeout_list = list_next;
		/* call callback */
		callback(user_data);
	}
	(void)ecode;
}

void timer_init(void) {
	timer_list = 0;
	timer_data_pool = 0;
	io_out8(0x43, 0x34); /* counter 0, LSB->MSB write, pulse generation, binary */
	io_out8(0x40, 0xa9);
	io_out8(0x40, 0x04); /* 1/1193 -> about 1000Hz */
	set_interrupt_handler(0x20, timer_interrupt_handler);
	set_irq_enable(get_irq_enabled() | 0x01);
}

void timer_set(int timeout, void (*callback)(void*), void* user_data) {
	struct timer_data_t* next_node, **itr;
	int eflags;
	int push;
	/* disable interrupt */
	__asm__ __volatile__(
		"pushf\n\t"
		"cli\n\t"
		"pop %%eax\n\t"
		"mov %%eax, %0\n\t"
	: "=m"(eflags) : : "%eax");

	/* create new node if no new nodes are left */
	if (timer_data_pool == 0) {
		struct timer_data_t* new_memory = memory_allocate(0x1000);
		int num = 0x1000 / sizeof(*new_memory);
		int i;
		for (i = 0; i < num; i++) {
			new_memory[i].next = timer_data_pool;
			timer_data_pool = &new_memory[i];
		}
	}

	/* get a node */
	next_node = timer_data_pool;
	timer_data_pool = timer_data_pool->next;

	/* initialize the new node */
	next_node->timeout = timeout;
	next_node->timeout_push = 0;
	next_node->callback = callback;
	next_node->user_data = user_data;

	/* insert the node */
	itr = &timer_list;
	push = 0;
	while (*itr != 0) {
		(*itr)->timeout -= push;
		(*itr)->timeout_push += push;
		push = (*itr)->timeout_push;
		if (timeout <= (*itr)->timeout) break;
		(*itr)->timeout_push = 0;
		itr = &(*itr)->next;
	}
	next_node->next = *itr;
	*itr = next_node;

	/* enable interrupt only if previously enabled */
	__asm__ __volatile__(
		"testl $0x0200, %0\n\t"
		"jz 1f\n\t"
		"sti\n\t"
		"1:\n\t"
	: : "m"(eflags));
}
