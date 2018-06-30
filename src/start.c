#include "gdt.h"
#include "memory.h"
#include "interrupts.h"
#include "serial.h"
#include "display.h"
#include "terminal.h"
#include "keyboard.h"
#include "read_input.h"
#include "thread_switch.h"
#include "timer.h"

#ifdef NATIVE_HACK
void __chkstk_ms(void) {}
#endif

struct cpu_status_t main_thread, sub_thread;

int fib(int n) {
	if (n <= 1) return n;
	return fib(n - 1) + fib(n - 2);
}

void sub_thread_func(void) {
	volatile int r;
	*((volatile char*)0x10001) = 0; /* marker */
	r = fib(10);
	*((volatile char*)0x10001) = 0; /* marker */
	switch_thread(&sub_thread, &main_thread);
}

void timer_callback(void* data) {
	const char* s = (const char*)data;
	while (*s != '\0') {
		terminal_putchar(*s);
		s++;
	}
	terminal_putchar('\r');
	terminal_putchar('\n');
}

int _start(void* arg1) {
	volatile int marker = 0xDEADBEEF;
	const char* str = "hello, world\r\n";
	/* avoid unused warnings */
	(void)arg1;
	(void)marker;

	gdt_init();
	memory_init();
	display_init();
	interrupts_init();
	terminal_init();
	keyboard_init();
	timer_init();
	read_input_init();
	serial_init();
	serial_write(0x1b); serial_write('c'); /* VT100 reset */

	/* memory allocation test */
	{
		volatile char *marker = (volatile char*)0x10000;
		void *buf1, *buf2, *buf3, *buf4;
		*marker = 0;
		buf4 = memory_allocate(0x1000);
		buf3 = memory_allocate(0x1000);
		buf2 = memory_allocate(0x1000);
		buf1 = memory_allocate(0x1000);
		*marker = 0;
		memory_free(buf1);
		*marker = 0;
		memory_free(buf4);
		*marker = 0;
		memory_free(buf3);
		*marker = 0;
		memory_free(buf2);
		*marker = 0;
	}

	/* thread switch test */
	{
		void* new_stack = memory_allocate(64 * 1024);
		sub_thread.eax = 0xdeadbeef;
		sub_thread.ecx = 0x12345678;
		sub_thread.edx = 0x9abcdef0;
		sub_thread.ebx = 0xbbbbbbbb;
		sub_thread.esi = 0x55555555;
		sub_thread.edi = 0xaaaaaaaa;
		sub_thread.esp = sub_thread.ebp = (unsigned int)new_stack + (64 * 1024) - 4;
		sub_thread.cs = 0x08;
		sub_thread.ds = sub_thread.es = sub_thread.ss = sub_thread.fs = sub_thread.gs = 0x10;
		sub_thread.eflags = 0x00000002;
		sub_thread.eip = (unsigned int)sub_thread_func;
		*((volatile char*)0x10001) = 0; /* marker */
		switch_thread(&main_thread, &sub_thread);
		*((volatile char*)0x10001) = 0; /* marker */
		memory_free(new_stack);
	}

	/* timer test */
	timer_set(1000, timer_callback, "1s");
	timer_set(3000, timer_callback, "3s");
	timer_set(2000, timer_callback, "2s");
	timer_set(10000, timer_callback, "10s");
	timer_set(5000, timer_callback, "5s");

	while (*str != '\0') {
		terminal_putchar(*str);
		serial_write(*str);
		str++;
	}
	for (;;) {
		unsigned char buf[4096];
		int len, i;
		terminal_putchar('Y');
		terminal_putchar('U');
		terminal_putchar('K');
		terminal_putchar('I');
		terminal_putchar('.');
		terminal_putchar('N');
		terminal_putchar('>');
		for (len = 0; len < 4096; len++) {
			int c = read_input_char();
			if (c == '\n') break;
			buf[len++] = c;
		}
		for (i = 0; i < len; i++) {
			terminal_putchar(buf[i]);
			serial_write(buf[i]);
		}
		terminal_putchar('\r'); serial_write('\r');
		terminal_putchar('\n'); serial_write('\n');
	}

	return 0;
}
