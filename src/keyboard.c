#include "keyboard.h"
#include "interrupts.h"
#include "io.h"

#define KEYBOARD_BUFFER_SIZE 256

unsigned char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
volatile int kbuf_start, kbuf_end = 0;

static void keyboard_hw_write(int to_encoder, int value) {
	while (io_in8(0x64) & 0x02); /* wait until input buffer empty */
	io_out8(to_encoder ? 0x60 : 0x64, value & 0xff);
}

void keyboard_interrupt_handler(int ecode) {
	int kbuf_next = (kbuf_end + 1) % KEYBOARD_BUFFER_SIZE;
	if (kbuf_next != kbuf_start) {
		keyboard_buffer[kbuf_end] = io_in8(0x60);
		kbuf_end = kbuf_next;
	}
	(void)ecode;
}

static int keyboard_hw_read(void) {
	while (!(io_in8(0x64) & 0x01)); /* wait until output buffer full */
	return io_in8(0x60);
}

void keyboard_init(void) {
	int ret;
	/* enable keyboard, disable mouse, disable interrupt */
	keyboard_hw_write(0, 0x60); /* write RAM */
	keyboard_hw_write(1, 0x20); /* data to write */

	/* keyboard reset */
	keyboard_hw_write(1, 0xff);
	ret = keyboard_hw_read();
	if (ret != 0xfa) {
		return;
	}
	ret = keyboard_hw_read();
	if (ret != 0xaa) {
		return;
	}

	/* set scan code 2 */
	keyboard_hw_write(1, 0xf0);
	keyboard_hw_write(1, 0x02);
	ret = keyboard_hw_read();
	if (ret != 0xfa) {
		return;
	}

	/* enable interrupt */
	kbuf_start = kbuf_end = 0;
	set_interrupt_handler(0x21, keyboard_interrupt_handler);
	keyboard_hw_write(0, 0x60); /* write RAM */
	keyboard_hw_write(1, 0x21); /* data to write */
	set_irq_enable(get_irq_enabled() | 0x02);
}

int keyboard_read(void) {
	int ret;
	while (kbuf_start == kbuf_end); /* wait until the queue has some data */
	ret = keyboard_buffer[kbuf_start];
	kbuf_start = (kbuf_start + 1) % KEYBOARD_BUFFER_SIZE;
	return ret;
}
