#include "gdt.h"
#include "interrupts.h"
#include "serial.h"
#include "display.h"
#include "terminal.h"

int _start(void* arg1) {
	volatile int marker = 0xDEADBEEF;
	const char* str = "hello, world\r\n";
	/* avoid unused warnings */
	(void)arg1;
	(void)marker;

	gdt_init();
	display_init();
	interrupts_init();
	terminal_init();
	serial_init();
	serial_write(0x1b); serial_write('c'); /* VT100 reset */

	while (*str != '\0') {
		terminal_putchar(*str);
		serial_write(*str);
		str++;
	}
	while (serial_read() != 'q');

	return 0;
}
