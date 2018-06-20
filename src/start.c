#include "gdt.h"
#include "interrupts.h"
#include "serial.h"
#include "display.h"
#include "io.h"

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
	const char* gstr = "hello, world";
	unsigned char* vram = (unsigned char*)0xb8000;
	int i;
	/* avoid unused warnings */
	(void)arg1;
	(void)marker;

	gdt_init();
	set_interrupt_handler(ihandler);
	interrupts_init();
	display_init();
	serial_init();

	io_out16(0x03c4, 0x0204);
	io_out16(0x03ce, 0x1305);
	io_out16(0x03ce, 0x0e06);
	for (i = 0; gstr[i] != '\0'; i++) {
		vram[i * 2] = gstr[i];
	}
	for(i=800;i<0x8000;i+=801)vram[i * 2] = '0';

	while (*str != '\0') {
		serial_write(*(str++));
	}
	while (serial_read() != 'q');

	return 0;
}
