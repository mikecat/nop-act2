#include "terminal.h"
#include "display.h"

#define WIDTH 80
#define HEIGHT 25

#define ESC_PARAM_SIZE_MAX 128
#define ESC_PARAM_NUM_MAX 16

static unsigned char* vram = (unsigned char*)0xb8000;

struct char_element_t {
	unsigned char c;
	unsigned char attr;
};

int cursor_x, cursor_y;
struct char_element_t display[HEIGHT][WIDTH];

int rightmost_print_mode;

int escmode;
int escchar;
enum esc_status_t {
	ESC_INIT,
	ESC_TWOCHAR,
	ESC_MULTICHAR
} esctype;
char escparam[ESC_PARAM_SIZE_MAX];
int escparamlen;

void terminal_init(void) {
	int i, j;
	cursor_x = 0;
	cursor_y = 0;
	for (i = 0; i < HEIGHT; i++) {
		for (j = 0; j < WIDTH; j++) {
			vram[(i * WIDTH + j) * 2    ] = display[i][j].c = ' ';
			vram[(i * WIDTH + j) * 2 + 1] = display[i][j].attr = 0x07;
		}
	}
	move_cursor(cursor_x, cursor_y);
	escmode = 0;
	escchar = 0;
	esctype = ESC_INIT;
	escparamlen = 0;
	rightmost_print_mode = 0;
}

static void screen_shiftup(void) {
	int i, j;
	for (i = 1; i < HEIGHT; i++) {
		for (j = 0; j < WIDTH; j++) {
			vram[((i - 1) * WIDTH + j) * 2    ] = display[i - 1][j].c = display[i][j].c;
			vram[((i - 1) * WIDTH + j) * 2 + 1] = display[i - 1][j].attr = display[i][j].attr;
		}
	}
	for (j = 0; j < WIDTH; j++) {
		vram[((HEIGHT - 1) * WIDTH + j) * 2    ] = display[HEIGHT - 1][j].c = ' ';
		vram[((HEIGHT - 1) * WIDTH + j) * 2 + 1] = display[HEIGHT - 1][j].attr = 0x07;
	}
}

static void screen_shiftdown(void) {
	int i, j;
	for (i = HEIGHT - 1; i > 0; i--) {
		for (j = 0; j < WIDTH; j++) {
			vram[(i * WIDTH + j) * 2    ] = display[i][j].c = display[i - 1][j].c;
			vram[(i * WIDTH + j) * 2 + 1] = display[i][j].attr = display[i - 1][j].attr;
		}
	}
	for (j = 0; j < WIDTH; j++) {
		vram[j * 2    ] = display[0][j].c = ' ';
		vram[j * 2 + 1] = display[0][j].attr = 0x07;
	}
}

static void esc_onechar(int c) {
	switch (c) {
		case 'E':
			cursor_x = 0;
			/* do same thing as case 'D' */
		case 'D':
			if (cursor_y < HEIGHT - 1) {
				cursor_y++;
			} else {
				screen_shiftup();
			}
			move_cursor(cursor_x, cursor_y);
			break;
		case 'M':
			if (cursor_y > 0) {
				cursor_y--;
				move_cursor(cursor_x, cursor_y);
			} else {
				screen_shiftdown();
			}
			break;
		case 'c':
			{
				int i, j;
				for (i = 0; i < HEIGHT; i++) {
					for (j = 0; j < WIDTH; j++) {
						vram[(i * WIDTH + j) * 2] = display[i][j].c = ' ';
					}
				}
				cursor_x = cursor_y = 0;
				move_cursor(cursor_x, cursor_y);
			}
			break;
	}
}

static void esc_twochar(int c1, int c2) {
	switch (c1) {
		case '#':
			switch (c2) {
				case '8':
					{
						int i, j;
						for (i = 0; i < HEIGHT; i++) {
							for (j = 0; j < WIDTH; j++) {
								vram[(i * WIDTH + j) * 2] = display[i][j].c = 'E';
							}
						}
						cursor_x = cursor_y = 0;
						move_cursor(cursor_x, cursor_y);
					}
					break;
			}
			break;
	}
}

static void esc_multichar(int c) {
	int params[ESC_PARAM_NUM_MAX];
	int param_num = 0;
	int question = 0;
	int i = 0;
	int current = -1;
	if (escparamlen > 0 && escparam[0] == '?') {
		question = 1;
		i = 1;
	}
	for (; i < escparamlen; i++) {
		if ('0' <= escparam[i] && escparam[i] <= '9') {
			int digit = escparam[i] - '0';
			if (current < 0) current = 0;
			if (current > 0x7fffffff / 10 || current * 10 > 0x7fffffff - digit) {
				/* overflow */
				current = 0x7fffffff;
			} else {
				current = current * 10 + digit;
			}
		} else if (escparam[i] == ';') {
			if (param_num < ESC_PARAM_NUM_MAX) params[param_num++] = current;
			current = -1;
		}
	}
	if (param_num < ESC_PARAM_NUM_MAX) params[param_num++] = current;
	if (!question) {
		switch (c) {
			case 'A':
				{
					int delta = params[0] < 0 ? 1 : params[0];
					if (cursor_y < delta) cursor_y = 0; else cursor_y -= delta;
					move_cursor(cursor_x, cursor_y);
				}
				break;
			case 'B':
				{
					int delta = params[0] < 0 ? 1 : params[0];
					if (delta > HEIGHT - cursor_y - 1) cursor_y = HEIGHT - 1; else cursor_y += delta;
					move_cursor(cursor_x, cursor_y);
				}
				break;
			case 'C':
				{
					int delta = params[0] < 0 ? 1 : params[0];
					if (delta > WIDTH - cursor_x - 1) cursor_x = WIDTH - 1; else cursor_x += delta;
					move_cursor(cursor_x, cursor_y);
				}
				break;
			case 'D':
				{
					int delta = params[0] < 0 ? 1 : params[0];
					if (cursor_x < delta) cursor_x = 0; else cursor_x -= delta;
					move_cursor(cursor_x, cursor_y);
				}
				break;
			case 'H':
			case 'f':
				cursor_y = params[0] - 1;
				cursor_x = param_num >= 2 ? params[1] - 1 : 0;
				if (cursor_y < 0) cursor_y = 0;
				if (cursor_y >= HEIGHT) cursor_y = HEIGHT - 1;
				if (cursor_x < 0) cursor_x = 0;
				if (cursor_x >= WIDTH) cursor_x = WIDTH - 1;
				move_cursor(cursor_x, cursor_y);
				break;
		}
	}
}

void terminal_putchar(int c) {
	if (escmode) {
		if (c == 0x1b) {
			escmode = 1;
			escchar = 0;
			esctype = ESC_INIT;
			return;
		} else if (c == 0x18 || c == 0x1a) {
			escmode = 0;
			rightmost_print_mode = 0;
			return;
		} else if (0x20 <= c && c < 0x7f) {
			switch (esctype) {
				case ESC_INIT:
					if (c == '#' || c == '(' || c == ')') {
						/* escape sequence with two characters */
						esctype = ESC_TWOCHAR;
						escchar = c;
					} else if (c == '[') {
						/* escape sequence with multi characters */
						esctype = ESC_MULTICHAR;
						escchar = c;
						escparamlen = 0;
					} else {
						/* escape sequence with one character */
						esc_onechar(c);
						escmode = 0;
						rightmost_print_mode = 0;
					}
					break;
				case ESC_TWOCHAR:
					esc_twochar(escchar, c);
					escmode = 0;
					rightmost_print_mode = 0;
					break;
				case ESC_MULTICHAR:
					if (('0' <= c && c <= '9') || c == ';' || c == '?') {
						if (escparamlen < ESC_PARAM_SIZE_MAX) escparam[escparamlen++] = c;
					} else {
						esc_multichar(c);
						escmode = 0;
						rightmost_print_mode = 0;
					}
					break;
				default:
					/* error */
					escmode = 0;
					rightmost_print_mode = 0;
					break;
			}
			return;
		}
	}
	switch(c) {
		case '\r':
			cursor_x = 0;
			move_cursor(cursor_x, cursor_y);
			rightmost_print_mode = 0;
			break;
		case '\n':
			if (cursor_y < HEIGHT - 1) {
				cursor_y++;
				move_cursor(cursor_x, cursor_y);
			} else {
				screen_shiftup();
			}
			rightmost_print_mode = 0;
			break;
		case '\b':
			if (cursor_x > 0) {
				cursor_x--;
				move_cursor(cursor_x, cursor_y);
			}
			rightmost_print_mode = 0;
			break;
		case 0x1b:
			escmode = 1;
			escchar = 0;
			esctype = ESC_INIT;
			break;
		default:
			if (0x20 <= c && c < 0x7f) {
				if (0 <= cursor_x && cursor_x < WIDTH && 0 <= cursor_y && cursor_y < HEIGHT) {
					if (!rightmost_print_mode) {
						vram[(cursor_y * WIDTH + cursor_x) * 2] = display[cursor_y][cursor_x].c = c;
					}
					if (cursor_x + 1 < WIDTH) {
						cursor_x++;
						rightmost_print_mode = 0;
					} else {
						if (rightmost_print_mode) {
							if (cursor_y + 1 < HEIGHT) {
								cursor_y++;
								cursor_x = 0;
							} else {
								screen_shiftup();
								cursor_x = 0;
							}
							vram[(cursor_y * WIDTH + cursor_x) * 2] = display[cursor_y][cursor_x].c = c;
							cursor_x++;
							rightmost_print_mode = 0;
						} else {
							rightmost_print_mode = 1;
						}
					}
					move_cursor(cursor_x, cursor_y);
				}
			}
			break;
	}
}
