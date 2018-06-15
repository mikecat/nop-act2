#include "interrupts.h"
#include "gdt.h"

extern void* interrupts_land_table[];
static void (*interrupt_handler)(int iid, int ecode) = 0;

unsigned char idt[8 * 256];

void interrupts_init(void) {
	unsigned char lidt[6];
	unsigned long idt_addr = (unsigned long)idt;
	int i;
	for (i = 0; i < 256; i++) {
		unsigned long addr = (unsigned long)interrupts_land_table[i];
		idt[8 * i + 0] = addr & 0xff;
		idt[8 * i + 1] = (addr >> 8) & 0xff;
		idt[8 * i + 2] = SEGMENT_CODE_0 & 0xff;
		idt[8 * i + 3] = (SEGMENT_CODE_0 >> 8) & 0xff;
		idt[8 * i + 4] = 0x00;
		idt[8 * i + 5] = 0x8e;
		idt[8 * i + 6] = (addr >> 16) & 0xff;
		idt[8 * i + 7] = (addr >> 24) & 0xff;
	}
	lidt[0] = sizeof(idt)  & 0xff;
	lidt[1] = (sizeof(idt) >> 8) & 0xff;
	lidt[2] = idt_addr & 0xff;
	lidt[3] = (idt_addr >> 8) & 0xff;
	lidt[4] = (idt_addr >> 16) & 0xff;
	lidt[5] = (idt_addr >> 24) & 0xff;
	__asm__ __volatile__ (
		"lidt %0\n\t"
	: : "m"(lidt));
}

void set_interrupt_handler(void (*handler)(int iid, int ecode)) {
	interrupt_handler = handler;
}

void c_base_interrupt_handler(int iid, int ecode) {
	if (interrupt_handler != 0) interrupt_handler(iid, ecode);
}
