	.file	"add.c"
	.text
	.section	.rodata
.LC0:
	.string	"The sum of %d and %d is %d\n"	; first string
.LC1:
	.string	"The sum of 20 and 30 is %d\n"	; second string
	.text
	.globl	main							; main function
	.type	main, @function
main:
.LFB0:
	.cfi_startproc							; debug
	endbr64									; Intel CET
	pushq	%rbp							; function prologue
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp						; set base pointer rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp						; allocate 16 bytes on stack for local vars
	movl	$5, -16(%rbp)					; alloc a = 5 at rbp-16 or 0 in local stack						c code: int a = 5
	movl	$10, -12(%rbp)					; alloc b = 10 at rbp-12 or 4 in local stack					c code: int b = 10
	movl	-16(%rbp), %edx					; move a into edx register
	movl	-12(%rbp), %eax					; move b into eax register
	addl	%edx, %eax						; add eax and edx, a and b										c code: a + b
	movl	%eax, -8(%rbp)					; move value from eax (sum) into rbp-8 or 8 in local stack		c code: sum = a + b
	movl	-8(%rbp), %ecx					; move sum into ecx
	movl	-12(%rbp), %edx					; move b into edx
	movl	-16(%rbp), %eax					; move a into eax
	movl	%eax, %esi						; move a into esi
	leaq	.LC0(%rip), %rax				; copy string address into rax
	movq	%rax, %rdi						; move string address into rdi
	movl	$0, %eax						; eax is set to number of vector registers used
	call	printf@PLT						; call printf function
	movl	$50, -4(%rbp)					; compiler optimized, already computed 20+30 so 50 is stored	c code: result = 20 + 30
	movl	-4(%rbp), %eax					; move result into eax
	movl	%eax, %esi						; move eax into esi
	leaq	.LC1(%rip), %rax				; copy string address into rax
	movq	%rax, %rdi						; move string address into rdi
	movl	$0, %eax						; set eax to # of vector registors used
	call	printf@PLT						; call printf function
	movl	$0, %eax						; set return val to 0
	leave									; sets rsp to rbp and pop rbp
	.cfi_def_cfa 7, 8						; prologue stuff
	ret										; return from function
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
