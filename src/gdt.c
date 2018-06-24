#include "gdt.h"

unsigned char gdt[] = {
	/* 00 : null */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 08 : ring 0 code */
	0xff, 0xff, /* Segment Limit Low */
	0x00, 0x00, /* Base Address Low */
	0x00, /* Base Address Mid */
	0x9a, /* P=1, DPL=0, S=1, Type={Code, Up, Read/Exec, Not accessed} */
	0xcf, /* G=1, D/B=1, AVL=0, Segment Limit High=0xf */
	0x00, /* Base Address Hi */
	/* 10 : ring 0 data */
	0xff, 0xff, /* Segment Limit Low */
	0x00, 0x00, /* Base Address Low */
	0x00, /* Base Address Mid */
	0x92, /* P=1, DPL=0, S=1, Type={Data, Up, Read/Write, Not accessed} */
	0xcf, /* G=1, D/B=1, AVL=0, Segment Limit High=0xf */
	0x00, /* Base Address Hi */
	/* 18 : ring 3 code */
	0xff, 0xff, /* Segment Limit Low */
	0x00, 0x00, /* Base Address Low */
	0x00, /* Base Address Mid */
	0xfa, /* P=1, DPL=3, S=1, Type={Code, Up, Read/Exec, Not accessed} */
	0xcf, /* G=1, D/B=1, AVL=0, Segment Limit High=0xf */
	0x00, /* Base Address Hi */
	/* 20 : ring 3 data */
	0xff, 0xff, /* Segment Limit Low */
	0x00, 0x00, /* Base Address Low */
	0x00, /* Base Address Mid */
	0xf2, /* P=1, DPL=3, S=1, Type={Data, Up, Read/Write, Not accessed} */
	0xcf, /* G=1, D/B=1, AVL=0, Segment Limit High=0xf */
	0x00, /* Base Address Hi */
	/* 28 : ring 0 code (16bit) */
	0xff, 0xff, /* Segment Limit Low */
	0x00, 0x00, /* Base Address Low */
	0x00, /* Base Address Mid */
	0x9a, /* P=1, DPL=0, S=1, Type={Code, Up, Read/Exec, Not accessed} */
	0x8f, /* G=1, D/B=0, AVL=0, Segment Limit High=0xf */
	0x00, /* Base Address Hi */
	/* 30 : ring 0 data (16bit) */
	0xff, 0xff, /* Segment Limit Low */
	0x00, 0x00, /* Base Address Low */
	0x00, /* Base Address Mid */
	0x92, /* P=1, DPL=0, S=1, Type={Data, Up, Read/Write, Not accessed} */
	0x8f, /* G=1, D/B=0, AVL=0, Segment Limit High=0xf */
	0x00, /* Base Address Hi */
};

void gdt_init(void) {
	unsigned char gdtr[6];
	unsigned long gdt_address = (unsigned long)gdt;
	gdtr[0] = sizeof(gdt) & 0xff;
	gdtr[1] = (sizeof(gdt) >> 8) & 0xff;
	gdtr[2] = gdt_address & 0xff;
	gdtr[3] = (gdt_address >> 8) & 0xff;
	gdtr[4] = (gdt_address >> 16) & 0xff;
	gdtr[5] = (gdt_address >> 24) & 0xff;
	__asm__ __volatile__ (
		"cli\n\t"
		"lgdt %0\n\t"
		"mov $0x10, %%ax\n\t"
		"mov %%ax, %%ds\n\t"
		"mov %%ax, %%es\n\t"
		"mov %%ax, %%fs\n\t"
		"mov %%ax, %%gs\n\t"
		"mov %%ax, %%ss\n\t"
		"ljmp $0x08, $1f\n\t"
		"1:\n\t"
		"sti:\n\t"
	: : "m"(gdtr));
}
