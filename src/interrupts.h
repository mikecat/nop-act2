#ifndef INTERRUPTS_H_GUARD_9288A427_E7C2_4197_9CDB_42A09351F41C
#define INTERRUPTS_H_GUARD_9288A427_E7C2_4197_9CDB_42A09351F41C

void interrupts_init(void);
void set_interrupt_handler(void (*handler)(int iid, int ecode));

#endif
