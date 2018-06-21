.section .text

#define JUMPER(two, one) \
	irand_ ## two ## one : \
		push $0x00 ; /* dummy error code */ \
		push $0x ## two ## one ; \
		jmp irand_caller ;

#define JUMPER_NOERR(two, one) \
	irand_ ## two ## one : \
		nop ; nop ; /* dummy error code */ \
		push $0x ## two ## one ; \
		jmp irand_caller ;

#define JUMPER_X(two) \
	JUMPER(two, 0) JUMPER(two, 1) JUMPER(two, 2) JUMPER(two, 3) \
	JUMPER(two, 4) JUMPER(two, 5) JUMPER(two, 6) JUMPER(two, 7) \
	JUMPER(two, 8) JUMPER(two, 9) JUMPER(two, a) JUMPER(two, b) \
	JUMPER(two, c) JUMPER(two, d) JUMPER(two, e) JUMPER(two, f) \

JUMPER(0, 0) JUMPER(0, 1) JUMPER(0, 2) JUMPER(0, 3)
JUMPER(0, 4) JUMPER(0, 5) JUMPER(0, 6) JUMPER(0, 7)
JUMPER_NOERR(0, 8) JUMPER(0, 9) JUMPER_NOERR(0, a) JUMPER_NOERR(0, b)
JUMPER_NOERR(0, c) JUMPER_NOERR(0, d) JUMPER_NOERR(0, e) JUMPER(0, f)

JUMPER(1, 0) JUMPER_NOERR(1, 1) JUMPER(1, 2) JUMPER(1, 3)
JUMPER(1, 4) JUMPER(1, 5) JUMPER(1, 6) JUMPER(1, 7)
JUMPER(1, 8) JUMPER(1, 9) JUMPER(1, a) JUMPER(1, b)
JUMPER(1, c) JUMPER(1, d) JUMPER(1, e) JUMPER(1, f)

JUMPER_X(2) JUMPER_X(3)
JUMPER_X(4) JUMPER_X(5) JUMPER_X(6) JUMPER_X(7)
JUMPER_X(8) JUMPER_X(9) JUMPER_X(a) JUMPER_X(b)
JUMPER_X(c) JUMPER_X(d) JUMPER_X(e) JUMPER_X(f)

irand_caller:
	pushal
	push 36(%esp) /* push error code */
	push 36(%esp) /* push vector number */
	call _c_base_interrupt_handler
	add $8, %esp /* remove arguments from the stack */
	popal
	add $8, %esp /* remove vector number and error code from the stack */
	iret

.section .data

.globl _interrupts_land_table
.globl interrupts_land_table /* for some environments */
_interrupts_land_table:
interrupts_land_table: /* for some environments */

#define TABLE(two, one) .long irand_ ## two ## one ;

#define TABLE_X(two) \
	TABLE(two, 0) TABLE(two, 1) TABLE(two, 2) TABLE(two, 3) \
	TABLE(two, 4) TABLE(two, 5) TABLE(two, 6) TABLE(two, 7) \
	TABLE(two, 8) TABLE(two, 9) TABLE(two, a) TABLE(two, b) \
	TABLE(two, c) TABLE(two, d) TABLE(two, e) TABLE(two, f) \

TABLE_X(0) TABLE_X(1) TABLE_X(2) TABLE_X(3)
TABLE_X(4) TABLE_X(5) TABLE_X(6) TABLE_X(7)
TABLE_X(8) TABLE_X(9) TABLE_X(a) TABLE_X(b)
TABLE_X(c) TABLE_X(d) TABLE_X(e) TABLE_X(f)
