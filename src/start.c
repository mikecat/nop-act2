#include "gdt.h"
#include "interrupts.h"
#include "serial.h"
#include "display.h"

int vbe_test(void) {
	extern int vbe_entry(void);
	extern unsigned char vbe_code[];
	extern unsigned int vbe_code_size;
	unsigned char *copy_ptr = (unsigned char*)0x1000;
	unsigned int i;
	for (i = 0; i < vbe_code_size; i++) copy_ptr[i] = vbe_code[i];
	return vbe_entry();
}

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
	int vbe_result;
	/* avoid unused warnings */
	(void)arg1;
	(void)marker;

	gdt_init();
	vbe_result = vbe_test(); /* test!!! */
	set_interrupt_handler(ihandler);
	interrupts_init();
	display_init();
	serial_init();

	for (i = 0; gstr[i] != '\0'; i++) {
		vram[i * 2] = gstr[i];
	}
#if 0
	for(i=800;i<0x8000;i+=801)vram[i * 2] = '0';
#else
	for(i=80;i<0x8000;i+=81)vram[i * 2] = '0';
#endif

	for (i = 0xc0000; i < 0x100000; i++) {
		if (*(unsigned int*)i == 0x504d4944
		|| *(unsigned int*)i == 0x44494d50) {
			__asm__ __volatile__(
				"mov %0, %%eax\n\t"
			: : "m"(i));
			for(;;);
		}
	}

	while (*str != '\0') {
		serial_write(*(str++));
	}
	serial_write(vbe_result + '0');
	while (serial_read() != 'q');

	return 0;
}
