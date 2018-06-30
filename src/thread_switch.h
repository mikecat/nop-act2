#ifndef THREAD_SWITCH_H_GUARD_E92252EB_D75F_451A_89AB_91F35D3A5EA2
#define THREAD_SWITCH_H_GUARD_E92252EB_D75F_451A_89AB_91F35D3A5EA2

struct cpu_status_t {
#if 1
	unsigned int eax, ecx, edx, ebx, esi, edi, esp, ebp;
	unsigned int eip, eflags;
	unsigned short cs, ds, es, ss, fs, gs;
#else
	char data[4 * 8 + 4 * 2 + 2 * 6];
#endif
};

void switch_thread(struct cpu_status_t* current, struct cpu_status_t* next);

#endif
