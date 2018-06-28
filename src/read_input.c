#include "read_input.h"
#include "terminal.h"
#include "keyboard.h"

#define INPUT_BUFFER_SIZE 4096

static unsigned char input_buffer[INPUT_BUFFER_SIZE];
static int input_buffer_next, input_buffer_len;

void read_input_init(void) {
	input_buffer_next = 0;
	input_buffer_len = 0;
}

static void send_number_to_terminal(int n) {
	char converted[16];
	char* c = converted;
	do {
		*(c++) = (n % 10) + '0';
		n /= 10;
	} while (n > 0);
	do {
		terminal_putchar(*(--c));
	} while (c > converted);
}

static void move_cursor(int dx, int dy) {
	if (dx != 0) {
		terminal_putchar(0x1b); terminal_putchar('[');
		send_number_to_terminal(dx > 0 ? dx : -dx);
		terminal_putchar(dx > 0 ? 'C' : 'D');
	}
	if (dy != 0) {
		terminal_putchar(0x1b); terminal_putchar('[');
		send_number_to_terminal(dy > 0 ? dy : -dy);
		terminal_putchar(dy > 0 ? 'B' : 'A');
	}
}

static void set_cursor(int x, int y) {
	terminal_putchar(0x1b); terminal_putchar('[');
	send_number_to_terminal(y); terminal_putchar(';');
	send_number_to_terminal(x); terminal_putchar('H');
}

int read_input_char(void) {
	if (input_buffer_next >= input_buffer_len) {
		int start_cx, start_cy;
		int scroll_top, scroll_bottom;
		int dummy;
		int limit_top, limit_bottom, limit_right;
		/* get initial cursor pos and scroll range */
		terminal_get_cursor_pos(&start_cx, &start_cy);
		move_cursor(0, -start_cy);
		terminal_get_cursor_pos(&dummy, &scroll_top);
		scroll_bottom = scroll_top;
		for (;;) {
			int next;
			move_cursor(0, 99);
			terminal_get_cursor_pos(&dummy, &next);
			if (next == scroll_bottom) break;
			scroll_bottom = next;
		}
		limit_right = start_cx;
		for (;;) {
			int next;
			move_cursor(99, 0);
			terminal_get_cursor_pos(&next, &dummy);
			if (next == limit_right) break;
			limit_right = next;
		}
		set_cursor(start_cx, start_cy);
		if (start_cy <= scroll_bottom) {
			if (start_cy < scroll_top) {
				limit_top = start_cy;
				limit_bottom = scroll_top - 1;
			} else {
				limit_top = scroll_top;
				limit_bottom = scroll_bottom;
			}
		} else {
			limit_top = start_cy;
			limit_bottom = start_cy;
			for (;;) {
				int next;
				move_cursor(0, 99);
				terminal_get_cursor_pos(&dummy, &next);
				if (next == limit_bottom) break;
				limit_bottom = next;
			}
			set_cursor(start_cx, start_cy);
		}
		/* read one line */
		{
			int buffer_cursor = 0;
			int terminal_cx = start_cx, terminal_cy = start_cy;
			int length_limit = (limit_bottom - terminal_cy) * limit_right
				+ (limit_right - terminal_cx + 1);
			input_buffer_next = 0;
			input_buffer_len = 0;
			for (;;) {
				int c = keyboard_read();
				if (c == '\n') {
					if (input_buffer_len < INPUT_BUFFER_SIZE) {
						input_buffer[input_buffer_len++] = '\n';
					}
					terminal_putchar('\r');
					terminal_putchar('\n');
					break;
				} else if (c == '\b') {
					if (buffer_cursor > 0 && input_buffer_len > 0) {
						int i;
						if (terminal_cx > 1) {
							terminal_cx--;
						} else {
							terminal_cx = limit_right;
							terminal_cy--;
						}
						set_cursor(terminal_cx, terminal_cy);
						for (i = buffer_cursor; i < input_buffer_len; i++) {
							terminal_putchar(input_buffer[i]);
							input_buffer[i - 1] = input_buffer[i];
						}
						terminal_putchar(' '); /* delete last character from terminal */
						buffer_cursor--;
						input_buffer_len--;
						set_cursor(terminal_cx, terminal_cy);
					}
				} else if (c == 0x7f) {
					if (input_buffer_len > 0) {
						int i;
						for (i = buffer_cursor + 1; i < input_buffer_len; i++) {
							terminal_putchar(input_buffer[i]);
							input_buffer[i - 1] = input_buffer[i];
						}
						terminal_putchar(' '); /* delete last character from terminal */
						input_buffer_len--;
						set_cursor(terminal_cx, terminal_cy);
					}
				} else if (c == 0x1b) {
					int cc = keyboard_read();
					if (cc == '[') {
						int d = -1;
						int d_reading = 1;
						for (;;) {
							cc = keyboard_read();
							if ('0' <= cc && cc <= '9') {
								if (d_reading) {
									if (d < 0) d = 0;
									d = d * 10 + (cc - '0');
								}
							} else if (cc == ';') {
								if (d_reading && d < 0) d = 1;
								d_reading = 0;
							} else {
								break;
							}
						}
						if (d_reading && d < 0) d = 1;
						if (cc == 'C') {
							int count;
							for (count = 0; count < d && buffer_cursor < input_buffer_len; count++) {
								if (terminal_cx < limit_right) {
									terminal_cx++;
								} else if (terminal_cy < limit_bottom) {
									terminal_cx = 1;
									terminal_cy++;
								} else if (start_cy > limit_top) {
									set_cursor(1, limit_bottom);
									terminal_putchar(0x1b); terminal_putchar('D');
									start_cy--;
									terminal_cx = 1;
								} else {
									break;
								}
								buffer_cursor++;
							}
							set_cursor(terminal_cx, terminal_cy);
						} else if (cc == 'D') {
							int count;
							for (count = 0; count < d && buffer_cursor > 0; count++) {
								buffer_cursor--;
								if (terminal_cx > 1) {
									terminal_cx--;
								} else {
									terminal_cx = limit_right;
									terminal_cy--;
								}
							}
							set_cursor(terminal_cx, terminal_cy);
						}
					}
				} else if (0x20 <= c && c < 0x7f) {
					if (input_buffer_len < INPUT_BUFFER_SIZE) {
						int i;
						if (input_buffer_len >= length_limit) {
							if (start_cy > limit_top) {
								set_cursor(1, limit_bottom);
								terminal_putchar(0x1b); terminal_putchar('D');
								terminal_cy--;
								start_cy--;
								set_cursor(terminal_cx, terminal_cy);
								length_limit += limit_right;
							} else {
								continue;
							}
						}
						for (i = input_buffer_len; i > buffer_cursor; i--) {
							input_buffer[i] = input_buffer[i - 1];
						}
						input_buffer[buffer_cursor] = c;
						input_buffer_len++;
						for (i = buffer_cursor; i < input_buffer_len; i++) {
							terminal_putchar(input_buffer[i]);
						}
						if (buffer_cursor >= input_buffer_len - 1 &&
						input_buffer_len >= length_limit && start_cy > limit_top) {
							set_cursor(1, limit_bottom);
							terminal_putchar(0x1b); terminal_putchar('D');
							terminal_cy--;
							start_cy--;
							length_limit += limit_right;
						}
						if (buffer_cursor < input_buffer_len - 1 ||
						terminal_cx < limit_right || input_buffer_len < length_limit) {
							buffer_cursor++;
							if (terminal_cx < limit_right) {
								terminal_cx++;
							} else {
								terminal_cx = 1;
								terminal_cy++;
							}
						}
						set_cursor(terminal_cx, terminal_cy);
					}
				}
			}
		}
	}
	return input_buffer[input_buffer_next++];
}
