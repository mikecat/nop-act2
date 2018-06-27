#include "keyboard.h"
#include "interrupts.h"
#include "io.h"

#define KEYBOARD_BUFFER_SIZE 256

/* scancode -> char {no-shift, shift} */
static char onechar_table[256][2] = {
	[0x1c] = "aA", [0x32] = "bB", [0x21] = "cC", [0x23] = "dD",
	[0x24] = "eE", [0x2b] = "fF", [0x34] = "gG", [0x33] = "hH",
	[0x43] = "iI", [0x3b] = "jJ", [0x42] = "kK", [0x4b] = "lL",
	[0x3a] = "mM", [0x31] = "nN", [0x44] = "oO", [0x4d] = "pP",
	[0x15] = "qQ", [0x2d] = "rR", [0x1b] = "sS", [0x2c] = "tT",
	[0x3c] = "uU", [0x2a] = "vV", [0x1d] = "wW", [0x22] = "xX",
	[0x35] = "yY", [0x1a] = "zZ",
	[0x45] = "00", [0x16] = "1!", [0x1e] = "2\"", [0x26] = "3#",
	[0x25] = "4$", [0x2e] = "5%", [0x36] = "6&", [0x3d] = "7'",
	[0x3e] = "8(", [0x46] = "9)", [0x4e] = "-=", [0x55] = "^~",
	[0x6a] = "\\|", [0x5b] = "[{", [0x5d] = "]}", [0x4c] = ";+",
	[0x52] = ":*", [0x41] = ",<", [0x49] = ".>", [0x4a] = "/?",
	[0x54] = "@`", [0x51] = "\\_",
	[0x5a] = "\n\n", [0x76] = "\x1b\x1b", [0x66] = "\b\b",
	[0x29] = "  ", [0x0d] = "\t\t",
	[0x70] = "00", [0x69] = "11", [0x72] = "22", [0x7a] = "33",
	[0x6b] = "44", [0x73] = "55", [0x74] = "66", [0x6c] = "77",
	[0x75] = "88", [0x7d] = "99", [0x71] = "..", [0x79] = "++",
	[0x7b] = "--", [0x7c] = "**"
};

unsigned char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
volatile int kbuf_start, kbuf_end = 0;

static void keyboard_enqueue(int c) {
	int kbuf_next = (kbuf_end + 1) % KEYBOARD_BUFFER_SIZE;
	if (kbuf_next != kbuf_start) {
		keyboard_buffer[kbuf_end] = c;
		kbuf_end = kbuf_next;
	}
}

int e0_flag, f0_flag;
int lshift_flag, rshift_flag;
int lctrl_flag, rctrl_flag;

static void keyboard_hw_write(int to_encoder, int value) {
	while (io_in8(0x64) & 0x02); /* wait until input buffer empty */
	io_out8(to_encoder ? 0x60 : 0x64, value & 0xff);
}

void keyboard_interrupt_handler(int ecode) {
	int sc = io_in8(0x60);
	if (sc == 0xe0) e0_flag = 1;
	else if (sc == 0xf0) f0_flag = 1;
	else {
		if (!e0_flag && sc == 0x12) lshift_flag = !f0_flag;
		else if (!e0_flag && sc == 0x59) rshift_flag = !f0_flag;
		else if (sc == 0x14) {
			if (e0_flag) rctrl_flag = !f0_flag; else lctrl_flag = !f0_flag;
		} else if (!f0_flag) {
			if (e0_flag) {
				if (sc == 0x4a) {
					if (!(lctrl_flag || rctrl_flag)) keyboard_enqueue('/');
				} else if (sc == 0x5a) {
					if (!(lctrl_flag || rctrl_flag)) keyboard_enqueue('\n');
				} else if (sc == 0x71) {
					if (!(lctrl_flag || rctrl_flag || lshift_flag || rshift_flag)) keyboard_enqueue(0x7f);
				} else {
					int code = 0;
					switch (sc) {
						case 0x75: code = 'A'; break; /* up */
						case 0x72: code = 'B'; break; /* down */
						case 0x74: code = 'C'; break; /* right */
						case 0x6b: code = 'D'; break; /* left */
					}
					if (!(lctrl_flag || rctrl_flag) && code != 0) {
						keyboard_enqueue(0x1b);
						keyboard_enqueue('[');
						keyboard_enqueue(code);
					}
				}
			} else {
				int c = onechar_table[sc][lshift_flag || rshift_flag];
				if (c != 0) {
					if (lctrl_flag || rctrl_flag) {
						if (c == '`') {
							keyboard_enqueue(0x1b);
						} else if (sc == 0x4e) {
							keyboard_enqueue(0x1f);
						} else if (c == 0x40 || ('a' <= c && c <= 'z') || (0x5b <= c && c <= 0x5f)) {
							keyboard_enqueue(c & 0x1f);
						}
					} else {
						keyboard_enqueue(c);
					}
				}
			}
		}
		e0_flag = 0;
		f0_flag = 0;
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
	ret = keyboard_hw_read();
	if (ret != 0xfa) {
		return;
	}
	keyboard_hw_write(1, 0x02);
	ret = keyboard_hw_read();
	if (ret != 0xfa) {
		return;
	}

	/* enable interrupt */
	kbuf_start = kbuf_end = 0;
	e0_flag = f0_flag = 0;
	lshift_flag = rshift_flag = 0;
	lctrl_flag = rctrl_flag = 0;
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
