.section .text

#define EAX_OFFSET 0
#define ECX_OFFSET 4
#define EDX_OFFSET 8
#define EBX_OFFSET 12
#define ESI_OFFSET 16
#define EDI_OFFSET 20
#define ESP_OFFSET 24
#define EBP_OFFSET 28
#define EIP_OFFSET 32
#define EFLAGS_OFFSET 36
#define CS_OFFSET 40
#define DS_OFFSET 42
#define ES_OFFSET 44
#define SS_OFFSET 46
#define FS_OFFSET 48
#define GS_OFFSET 50

.globl _switch_thread
.globl switch_thread
_switch_thread:
switch_thread:
	push %ebx
	pushf
	mov 12(%esp), %ebx /* pointer to the strucure where current status is saved */
	test %ebx, %ebx
	jz switch_thread_save_end
	mov %eax, EAX_OFFSET(%ebx)
	mov %ecx, ECX_OFFSET(%ebx)
	mov %edx, EDX_OFFSET(%ebx)
	mov 4(%esp), %eax
	mov %eax, EBX_OFFSET(%ebx)
	mov %esi, ESI_OFFSET(%ebx)
	mov %edi, EDI_OFFSET(%ebx)
	lea 8(%esp), %eax
	mov %eax, ESP_OFFSET(%ebx)
	mov %ebp, EBP_OFFSET(%ebx)
	mov %cs, %ax
	mov %ax, CS_OFFSET(%ebx)
	mov %ds, %ax
	mov %ax, DS_OFFSET(%ebx)
	mov %es, %ax
	mov %ax, ES_OFFSET(%ebx)
	mov %ss, %ax
	mov %ax, SS_OFFSET(%ebx)
	mov %fs, %ax
	mov %ax, FS_OFFSET(%ebx)
	mov %gs, %ax
	mov %ax, GS_OFFSET(%ebx)
	mov (%esp), %eax
	mov %eax, EFLAGS_OFFSET(%ebx)
	call switch_thread_save_eip
	ret /* return from this function after returning from another thread */
switch_thread_save_eip:
	pop %eax
	mov %eax, EIP_OFFSET(%ebx)
switch_thread_save_end:
	mov 16(%esp), %ebx /* pointer to the strucure where next status is saved */
	test %ebx, %ebx
	jnz switch_thread_start_jump
	/* no next status to jump */
	add $8, %esp
	ret
switch_thread_start_jump:
	mov ECX_OFFSET(%ebx), %ecx
	mov EDX_OFFSET(%ebx), %edx
	mov ESI_OFFSET(%ebx), %esi
	mov EDI_OFFSET(%ebx), %edi
	mov ESP_OFFSET(%ebx), %esp
	mov EBP_OFFSET(%ebx), %ebp
	mov ES_OFFSET(%ebx), %ax
	mov %ax, %es
	mov SS_OFFSET(%ebx), %ax
	mov %ax, %ss
	mov FS_OFFSET(%ebx), %ax
	mov %ax, %fs
	mov GS_OFFSET(%ebx), %ax
	mov %ax, %gs
	mov EFLAGS_OFFSET(%ebx), %eax
	push %eax
	popf
	mov CS_OFFSET(%ebx), %ax
	movzwl %ax, %eax
	push %eax
	mov EIP_OFFSET(%ebx), %eax
	push %eax
	mov EBX_OFFSET(%ebx), %eax
	push %eax
	mov EAX_OFFSET(%ebx), %eax
	push %eax
	mov DS_OFFSET(%ebx), %ax
	mov %ax, %ds
	pop %eax
	pop %ebx
	lret /* pop %eip; pop %cs */
