#ifndef TIMER_H_GUARD_B08570D8_C2B2_40DA_BC5A_A3ABB60EF380
#define TIMER_H_GUARD_B08570D8_C2B2_40DA_BC5A_A3ABB60EF380

void timer_init(void);
void timer_set(int timeout, void (*callback)(void*), void* user_data);

#endif
