#ifndef NUMBER_IO_H_GUARD_475F71EC_1D4E_4446_92D1_672408430AD2
#define NUMBER_IO_H_GUARD_475F71EC_1D4E_4446_92D1_672408430AD2

static inline unsigned int read_number(const void* p, int size) {
	int i;
	unsigned int ret = 0;
	for (i = 0; i < size; i++) {
		ret |= ((const unsigned char*)p)[i] << (i * 8);
	}
	return ret;
}

static inline void write_number(void* p, int size, unsigned int n) {
	int i;
	for (i = 0; i < size; i++) {
		((unsigned char*)p)[i] = (n >> (i * 8)) & 0xff;
	}
}

#endif
