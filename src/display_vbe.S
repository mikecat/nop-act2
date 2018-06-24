#include "display_vbe.h"

.section text

.global _vbe_entry
.global vbe_entry
_vbe_entry:
vbe_entry:
	cli
	lidt idt_data
	/* put callee-save registers */
	mov %ebx, 0x9000
	mov %esi, 0x9004
	mov %edi, 0x9008
	mov %esp, 0x900c
	mov %ebp, 0x9010
	/* switch to 16bit */
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
	.short 0x400
	.long 0

.global _vbe_code
.global vbe_code
_vbe_code:
vbe_code:
	mov $0x0000, %ax
	mov %ax, %es
	mov %ax, %ss
	mov $0x3000, %sp
	/* BIOS init (reference: https://github.com/qemu/vgabios/blob/master/vgabios.c ) */
	mov $0xc000, %ax
	mov %ax, %ds
	cmpw $0xaa55, 0x0
	jne bios_not_detected
	lcall $0xc000, $0x0003

	mov $0x0000, %ax
	mov %ax, %ds
	mov $0x4f00, %ax
	mov $0x2000, %di
	int $0x10
	cmp $0x004f, %ax
	jne vbe_not_supported
	testb $0x02, 0x200a
	jnz not_vga_compatible
	mov $(VBE_SUCCESS), %ax
	jmp goback
bios_not_detected:
	mov $(VBE_BIOS_NOT_DETECTED), %ax
	jmp goback
vbe_not_supported:
	mov $(VBE_NOT_SUPPORTED), %ax
	jmp goback
not_vga_compatible:
	mov $(VBE_NOT_VGA_COMPATIBLE), %ax
goback:
	/* go back to protected 32-bit mode */
	mov %cr0, %ecx
	or $0x00000001, %ecx
	mov %ecx, %cr0
	ljmp $0x08, $(vbe_protected - vbe_code + 0x1000)

.code32
vbe_protected:
	/* restore segment registers */
	mov $0x10, %cx
	mov %cx, %ds
	mov %cx, %es
	mov %cx, %ss
	mov %cx, %fs
	mov %cx, %gs
	/* restore callee-save registers */
	mov 0x9000, %ebx
	mov 0x9004, %esi
	mov 0x9008, %edi
	mov 0x900c, %esp
	mov 0x9010, %ebp
	/* return */
	movzx %ax, %eax
	ret

vbe_code_end:

.balign 4
.global _vbe_code_size
.global vbe_code_size
_vbe_code_size:
vbe_code_size:
	.long vbe_code_end - vbe_code
