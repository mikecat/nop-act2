#include "gdt.h"
#include "interrupts.h"
#include "serial.h"
#include "display.h"
#include "terminal.h"

void ihandler(int iid, int ecode) {
	static const char digits[] = "0123456789ABCDEF";
	serial_write('[');
	serial_write(digits[(iid >> 4) & 0xf]);
	serial_write(digits[iid & 0xf]);
	serial_write(',');
	serial_write(digits[(ecode >> 12) & 0xf]);
	serial_write(digits[(ecode >> 8) & 0xf]);
	serial_write(digits[(ecode >> 4) & 0xf]);
	serial_write(digits[ecode & 0xf]);
	serial_write(']');
}

int _start(void* arg1) {
	volatile int marker = 0xDEADBEEF;
	const char* str = "hello, world\r\n";
	/* avoid unused warnings */
	(void)arg1;
	(void)marker;

	gdt_init();
	display_init();
	set_interrupt_handler(ihandler);
	interrupts_init();
	terminal_init();
	serial_init();

	while (*str != '\0') {
		terminal_putchar(*str);
		serial_write(*str);
		str++;
	}
	while (serial_read() != 'q');

	return 0;
}
