#include "gdt.h"
#include "interrupts.h"
#include "serial.h"
#include "display.h"
#include "terminal.h"
#include "keyboard.h"
#include "read_input.h"

#ifdef NATIVE_HACK
void __chkstk_ms(void) {}
#endif

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
	keyboard_init();
	read_input_init();
	serial_init();
	serial_write(0x1b); serial_write('c'); /* VT100 reset */

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
