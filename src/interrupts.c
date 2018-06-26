#include "interrupts.h"
#include "gdt.h"
#include "io.h"

extern void* interrupts_land_table[];
static void (*interrupt_handler[256])(int ecode);

unsigned char idt[8 * 256];

void interrupts_init(void) {
	unsigned char lidt[6];
	unsigned long idt_addr = (unsigned long)idt;
	int i;
	__asm__ __volatile__ ("cli\n\t");

	for (i = 0; i < 256; i++) interrupt_handler[i] = 0;

	/* config slave 8259A */
	io_out8(0xa0, 0x11); /* ICW1 (command) */
	io_out8(0xa1, 0x28); /* ICW2 (data) */
	io_out8(0xa1, 0x02); /* ICW3 (data) */
	io_out8(0xa1, 0x01); /* ICW4 (data) */
	io_out8(0xa1, 0xff); /* OCW1 (mask = data) */
	/* config master 8259A */
	io_out8(0x20, 0x11); /* ICW1 (command) */
	io_out8(0x21, 0x20); /* ICW2 (data) */
	io_out8(0x21, 0x04); /* ICW3 (data) */
	io_out8(0x21, 0x01); /* ICW4 (data) */
	io_out8(0x21, 0xff); /* OCW1 (mask = data) */

	/* config IDT */
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
		"sti\n\t"
	: : "m"(lidt));
}

void set_interrupt_handler(int iid, void (*handler)(int ecode)) {
	if (0 <= iid && iid < 256) interrupt_handler[iid] = handler;
}

void set_irq_enable(int enable) {
	io_out8(0x21, ~enable & 0xff);
	io_out8(0xa1, ~(enable >> 8) & 0xff);
}

int get_irq_enabled(void) {
	int ret = 0;
	ret |= io_in8(0x21);
	ret |= io_in8(0xa1) << 8;
	return ~ret & 0xffff;
}

void c_base_interrupt_handler(int iid, int ecode) {
	if (interrupt_handler[iid] != 0) interrupt_handler[iid](ecode);

	/* send EOI */
	if (0x20 <= iid && iid < 0x28) {
		io_out8(0x20, 0x20);
	} else if (0x28 <= iid && iid < 0x30) {
		io_out8(0xa0, 0x20);
	}
}

/* for some environments */
void _c_base_interrupt_handler(int iid, int ecode) {
	c_base_interrupt_handler(iid, ecode);
}
