#ifndef INTERRUPTS_H_GUARD_9288A427_E7C2_4197_9CDB_42A09351F41C
#define INTERRUPTS_H_GUARD_9288A427_E7C2_4197_9CDB_42A09351F41C

void interrupts_init(void);
void set_interrupt_handler(int iid, void (*handler)(int ecode));

/* bit 0 = IRQ0, ..., bit 15 = IRQ15 */
/* each bit : 0 = disable interrupt, 1 = enable interrupt */
void set_irq_enable(int enable);
int get_irq_enabled(void);

#endif
