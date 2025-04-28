	.file	"simpleglobal.c"
	.text
	.globl	a						; declare a as global
	.data
	.align 4						; align a on 4 bytes
	.type	a, @object
	.size	a, 4
a:
	.long	5						; store 5 in a					c code: int a = 5
	.globl	b						; declare b as global
	.align 4						; align b on 4 bytes
	.type	b, @object
	.size	b, 4
b:
	.long	10						; store 10 in b					c code: int b = 10
	.text
	.globl	main					; function declaration
	.type	main, @function
main:
.LFB0:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6			; local vars not used, so no need for space alloc
	movl	a(%rip), %edx			; load global a in edx
	movl	b(%rip), %eax			; loab global b in eax
	addl	%edx, %eax				; add a and b
	movl	%eax, -4(%rbp)			; store result in rbp-4			c code: int sum = a + b
	movl	$0, %eax				; return 0
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.4.0-2ubuntu1~20.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
