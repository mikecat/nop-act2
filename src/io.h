#ifndef IO_H_GUARD_E12EC305_0A6E_4A53_A473_CC455163065C
#define IO_H_GUARD_E12EC305_0A6E_4A53_A473_CC455163065C

inline int io_in8(int addr) {
	int ret;
	__asm__ __volatile__ (
		"in %%dx, %%al\n\t"
		"movzbl %%al, %%eax\n\t"
	: "=a"(ret) : "d"(addr));
	return ret;
}

inline int io_in16(int addr) {
	int ret;
	__asm__ __volatile__ (
		"in %%dx, %%ax\n\t"
		"movzwl %%ax, %%eax\n\t"
	: "=a"(ret) : "d"(addr));
	return ret;
}

inline int io_in32(int addr) {
	int ret;
	__asm__ __volatile__ (
		"in %%dx, %%eax\n\t"
	: "=a"(ret) : "d"(addr));
	return ret;
}

inline void io_out8(int addr, int value) {
	__asm__ __volatile__ (
		"out %%al, %%dx\n\t"
	: : "d"(addr), "a"(value));
}

inline void io_out16(int addr, int value) {
	__asm__ __volatile__ (
		"out %%ax, %%dx\n\t"
	: : "d"(addr), "a"(value));
}

inline void io_out32(int addr, int value) {
	__asm__ __volatile__ (
		"out %%eax, %%dx\n\t"
	: : "d"(addr), "a"(value));
}

#endif
