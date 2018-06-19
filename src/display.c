#include "display.h"
#include "io.h"

static inline void crt_write(int index, int data) {
	io_out16(0x03d4, ((data & 0xff) << 8) | (index & 0xff));
}

/* display parameter from http://ele-tech.net/vga-doc1/ */

void display_init(void) {
	const int h_display = 640;
	const int h_blank1 = 16;
	const int h_sync = 96;
	const int h_blank2 = 48;

	const int v_display = 480;
	const int v_blank1 = 10;
	const int v_sync = 2;
	const int v_blank2 = 33;

	const int hor_total = (h_display + h_blank1 + h_sync + h_blank2) / 8 - 5;
	const int hor_dis_en_end = h_display / 8 - 1;
	const int hor_start_blank = h_display / 8;
	const int hor_end_blank = (h_blank1 + h_sync + h_blank2) / 8;
	const int hor_start_retrace = (h_display + h_blank1) / 8;
	const int hor_end_retrace = h_sync / 8;
	const int ver_total = v_display + v_blank1 + v_sync + v_blank2 - 2;
	const int ver_start_retrace = v_display + v_blank1;
	const int ver_retrace_end = v_sync;
	const int ver_dis_en_end = v_display - 1;
	const int ver_start_blank = v_display;
	const int ver_end_blank = v_blank1 + v_sync + v_blank2;

	__asm__ __volatile("cli\n\t");

	/* sequencer reset */
	io_out16(0x03c4, 0x0000);
	io_out16(0x03c4, 0x0100);
	/* Clocking Mode */
	io_out16(0x03c4, 0x0301);
	/* Miscellaneous Output Register */
	io_out8(0x03c2, 0xc3);

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
	crt_write(0x05, ((hor_end_blank & 0x20) << 2) | (hor_start_retrace & 0x1f));
	/* Vertical Total */
	crt_write(0x06, ver_total & 0xff);

	__asm__ __volatile("sti\n\t");
}
