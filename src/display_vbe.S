.section text

.global _vbe_entry
.global vbe_entry
_vbe_entry:
vbe_entry:
	lidt idt_data
	ljmp $0x28, $vbe_16bit

.code16
vbe_16bit:
	mov $0x30, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %fs
	mov %ax, %gs
	mov %cr0, %eax
	and $0xfffffffe, %eax
	mov %eax, %cr0
	ljmp $0x00, $0x1000

.section .data

idt_data:
	.long 0
	.short 0x400

.global _vbe_code
.global vbe_code
_vbe_code:
vbe_code:
	mov $0x0000, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov $0x3000, %sp
	mov $0x4f00, %ax
	mov $0x2000, %di
	int $0x10
	cli
halt_loop:
	jmp halt_loop
vbe_code_end:

.balign 4
.global _vbe_code_size
.global vbe_code_size
_vbe_code_size:
vbe_code_size:
	.long vbe_code_end - vbe_code
