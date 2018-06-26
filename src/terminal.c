#include "terminal.h"
#include "display.h"

#define WIDTH 80
#define HEIGHT 25

static unsigned char* vram = (unsigned char*)0xb8000;

struct char_element_t {
	unsigned char c;
	unsigned char attr;
};

int cursor_x, cursor_y;
struct char_element_t display[HEIGHT][WIDTH];

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

void terminal_putchar(int c) {
	switch(c) {
		case '\r':
			cursor_x = 0;
			move_cursor(cursor_x, cursor_y);
			break;
		case '\n':
			if (cursor_y < HEIGHT - 1) {
				cursor_y++;
				move_cursor(cursor_x, cursor_y);
			} else {
				screen_shiftup();
			}
			break;
		case '\b':
			if (cursor_x > 0) {
				cursor_x--;
				move_cursor(cursor_x, cursor_y);
			}
			break;
		default:
			if (0x20 <= c && c < 0x7f) {
				if (0 <= cursor_x && cursor_x < WIDTH && 0 <= cursor_y && cursor_y < HEIGHT) {
					vram[(cursor_y * WIDTH + cursor_x) * 2] = display[cursor_y][cursor_x].c = c;
					if (cursor_x + 1 < WIDTH) {
						cursor_x++;
					} else if (cursor_y + 1 < HEIGHT) {
						cursor_y++;
						cursor_x = 0;
					} else {
						screen_shiftup();
						cursor_x = 0;
					}
					move_cursor(cursor_x, cursor_y);
				}
			}
			break;
	}
}