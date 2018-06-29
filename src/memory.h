#ifndef MEMORY_H_GUARD_18C594BF_176C_4677_95B6_845CA7044787
#define MEMORY_H_GUARD_18C594BF_176C_4677_95B6_845CA7044787

void memory_init(void);
void* memory_allocate(unsigned int size);
void memory_free(void* addr);

#endif
