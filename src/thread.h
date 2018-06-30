#ifndef THREAD_H_GUARD_96869B5C_05C6_4816_AEFA_7B6357C035D3
#define THREAD_H_GUARD_96869B5C_05C6_4816_AEFA_7B6357C035D3

void thread_init(void);
int thread_create(void (*thread_entry)(void*), void* thread_arg, unsigned int stack_size);
void thread_yield(void);
void thread_join(int tid);
void thread_exit(void);

#endif
