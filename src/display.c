#include "display.h"
#include "io.h"
#include "font.h"

static int vbe_init(void) {
	extern int vbe_entry(void);
	extern unsigned char vbe_code[];
	extern unsigned int vbe_code_size;
	unsigned char *copy_ptr = (unsigned char*)0x1000;
	unsigned int i;
	for (i = 0; i < vbe_code_size; i++) copy_ptr[i] = vbe_code[i];
	return vbe_entry();
}

static inline void crt_write(int index, int data) {
	io_out16(0x03d4, ((data & 0xff) << 8) | (index & 0xff));
}

/* display parameter from http://ele-tech.net/vga-doc1/ */

void display_init(void) {
	const int h_display = 640;
	const int h_blank1 = 16;
	const int h_sync = 96;
	const int h_blank2 = 48;

	const int v_display = 475;
	const int v_blank0 = 5;
	const int v_blank1 = 10;
	const int v_sync = 2;
	const int v_blank2 = 33;

	const int char_scanlines = 19;

	const int hor_total = (h_display + h_blank1 + h_sync + h_blank2) / 8 - 5;
	const int hor_dis_en_end = h_display / 8 - 1;
	const int hor_start_blank = h_display / 8;
	const int hor_end_blank = (h_blank1 + h_sync + h_blank2) / 8;
	const int hor_start_retrace = (h_display + h_blank1) / 8;
	const int hor_end_retrace = h_sync / 8;
	const int ver_total = v_display + v_blank0 + v_blank1 + v_sync + v_blank2 - 2;
	const int ver_start_retrace = v_display + v_blank0 + v_blank1;
	const int ver_end_retrace = v_sync;
	const int ver_dis_en_end = v_display - 1;
	const int ver_start_blank = v_display + v_blank0;
	const int ver_end_blank = v_blank1 + v_sync + v_blank2 - 1;

	__asm__ __volatile("cli\n\t");

	/* disable VBE b y calling BIOS initialization and have VGA registers work properly */
	if (!vbe_init()) {
		/* failed, fallback */

		/* disable VBE for QEMU */
		/* reference: https://github.com/qemu/vgabios/blob/master/vbe.h */
		/* reference: http://hrb.osask.jp/wiki/?advance/QEMUVGA */
		io_out16(0x01ce, 0x0004);
		io_out16(0x01cf, 0x0000);

		/* magic for have VirtualBox disable VBE and work properly */
		/* reference: https://www.virtualbox.org/svn/vbox/trunk/src/VBox/Devices/Graphics/DevVGA.cpp */
		io_out16(0x03c4, 0x0007);
	}

	/* sequencer reset */
	io_out16(0x03c4, 0x0000);
	io_out16(0x03c4, 0x0100);
	/* Clocking Mode */
	io_out16(0x03c4, 0x0301);
	/* Map Mask */
	io_out16(0x03c4, 0x0f02);
	/* Characer Map Select */
	io_out16(0x03c4, 0x0003);
	/* Memory Mode */
	io_out16(0x03c4, 0x0604);

	/* Miscellaneous Output Register */
	io_out8(0x03c2, 0xc3);

	/* sequencer release reset */
	io_out16(0x03c4, 0x0300);

	/* CRT setting */
	/* enable write */
	crt_write(0x11, 0x22);
	/* Horizontal Total */
	crt_write(0x00, hor_total & 0xff);
	/* Horizontal Display-Enable End */
	crt_write(0x01, hor_dis_en_end & 0xff);
	/* Start Horizontal Blanking */
	crt_write(0x02, hor_start_blank & 0xff);
	/* End Horizontal Blanking */
	crt_write(0x03, 0x80 | (hor_end_blank & 0x1f));
	/* Start Horizontal Retrace Pulse */
	crt_write(0x04, hor_start_retrace & 0xff);
	/* End Horizontal Retrace */
	crt_write(0x05, ((hor_end_blank & 0x20) << 2) | (hor_end_retrace & 0x1f));
	/* Vertical Total */
	crt_write(0x06, ver_total & 0xff);
	/* Overflow */
	crt_write(0x07,
		((ver_total & 0x100) >> 8) |
		((ver_dis_en_end & 0x100) >> 7) |
		((ver_start_retrace & 0x100) >> 6) |
		((ver_start_blank & 0x100) >> 5) |
		0x10 |
		((ver_total & 0x200) >> 4) |
		((ver_dis_en_end & 0x200) >> 3) |
		((ver_start_retrace & 0x200) >> 2));
	/* Preset Row Scan */
	crt_write(0x08, 0x00);
	/* Maximum Scan Line */
	crt_write(0x09, 0x40 | ((char_scanlines - 1) & 0x1f) |
		((ver_start_blank & 0x200) >> 4));
	/* Cursor Start */
	crt_write(0x0a, ((char_scanlines - 3) & 0x1f));
	/* Cursor End */
	crt_write(0x0b, char_scanlines & 0x1f);
	/* Start Address High */
	crt_write(0x0c, 0x00);
	/* Start Address Low */
	crt_write(0x0d, 0x00);
	/* Cursor Location High */
	crt_write(0x0e, 0x00);
	/* Cursor Location Low */
	crt_write(0x0f, 0x00);
	/* Vertical Retrace Start */
	crt_write(0x10, ver_start_retrace & 0xff);
	/* Vertical Retrace End */
	crt_write(0x11, 0xb0 | (ver_end_retrace & 0x0f));
	/* Vertical Display Enable End */
	crt_write(0x12, ver_dis_en_end & 0xff);
	/* Offset */
	crt_write(0x13, ((h_display / 8) / 2) & 0xff);
	/* Underline Location */
	crt_write(0x14, char_scanlines & 0x1f);
	/* Start Vertical Blanking */
	crt_write(0x15, ver_start_blank & 0xff);
	/* End Vertical Blanking */
	crt_write(0x16, ver_end_blank & 0xff);
	/* CRT Mode Control */
	crt_write(0x17, 0xa3);
	/* Line Compare Register */
	crt_write(0x18, 0xff);

	/* VGA driver setting */
	/* Set/Reset */
	io_out16(0x03ce, 0x0f00);
	/* Enabls Set/Reset */
	io_out16(0x03ce, 0x0f01);
	/* Color Compare */
	io_out16(0x03ce, 0x0002);
	/* Data Rotate */
	io_out16(0x03ce, 0x0003);
	/* Read Map Select */
	io_out16(0x03ce, 0x0004);
	/* Graphics Mode */
	io_out16(0x03ce, 0x0305);
	/* Miscellaneous */
	io_out16(0x03ce, 0x0c06);
	/* Color Don't Care */
	io_out16(0x03ce, 0x0f07);
	/* Bit Mask */
	io_out16(0x03ce, 0xff08);

	/* Attribute setting */
	/* start */
	io_in8(0x03da);
	/* Internal Palette */
	{
		int i;
		for (i = 0; i <= 0x0f; i++) {
			io_out8(0x03c0, i); io_out8(0x03c0, i);
		}
	}
	/* Attribute Mode Control */
	io_out8(0x03c0, 0x10); io_out8(0x03c0, 0x00);
	/* Overscan Color */
	io_out8(0x03c0, 0x11); io_out8(0x03c0, 0x00);
	/* Color Plane Enable */
	io_out8(0x03c0, 0x12); io_out8(0x03c0, 0x0f);
	/* Horizontal PEL Panning */
	io_out8(0x03c0, 0x13); io_out8(0x03c0, 0x00);
	/* Color Select */
	io_out8(0x03c0, 0x14); io_out8(0x03c0, 0x00);
	/* end */
	io_in8(0x03da);
	io_out8(0x03c0, 0x20);

	/* DAC setting */
	/* Palette Address (write mode) */
	io_out8(0x03c8, 0x00);
	/* Palette Data */
	{
		int i;
		for (i = 0; i <= 0xff; i++) {
			io_out8(0x03c9, i & 0x01 ? 0x3f : 0x00); /* R */
			io_out8(0x03c9, i & 0x02 ? 0x3f : 0x00); /* G */
			io_out8(0x03c9, i & 0x04 ? 0x3f : 0x00); /* B */
		}
	}
	/* PEL Mask */
	io_out8(0x03c6, 0xff);

	/* initialize screen */
	{
		int i;
		char* vram = (char*)0xb8000;
		/* set font */
		io_out16(0x03c4, 0x0402);
		for (i = 0; i < 0x100; i++) {
			int j;
			for (j = 0; j < 32; j++) {
				vram[i * 32 + j] = (j < FONT_HEIGHT ? font_data[i][j] : 0);
			}
		}
		/* set attribute */
		io_out16(0x03c4, 0x0202);
		for (i = 0; i < 0x8000; i++) vram[i] = 0x07;
		/* clear text and let user write texts to display */
		io_out16(0x03c4, 0x0102);
		for (i = 0; i < 0x8000; i++) vram[i] = 0x00;
	}
	/* select character and attribute plane */
	io_out16(0x03c4, 0x0302);
	/* select even/odd addressing */
	io_out16(0x03c4, 0x0204);
	io_out16(0x03ce, 0x1305);
	io_out16(0x03ce, 0x0e06);
}
