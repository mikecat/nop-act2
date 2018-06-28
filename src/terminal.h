#ifndef TERMINAL_H_GUARD_AB869D2B_D7A5_42C1_A3D0_58741324CD06
#define TERMINAL_H_GUARD_AB869D2B_D7A5_42C1_A3D0_58741324CD06

void terminal_init(void);
void terminal_putchar(int c);
void terminal_get_cursor_pos(int* px, int* py);

#endif
