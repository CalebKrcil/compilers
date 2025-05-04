	.file	"basic.kt"
	.section	.rodata
	.align	8
.LCdouble_fmt:
	.string "%f\n"
.LCint_fmt:
	.string "%d\n"
	.text
	.globl	main
	.type	main, @function
main:
.LFB0:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movl	$5, -32(%rbp)
	movl	-32(%rbp), %eax
	movl	%eax, -8(%rbp)
	movl	$2, -40(%rbp)
	movl	-40(%rbp), %eax
	movl	%eax, -16(%rbp)
	movl	-8(%rbp), %eax
	addl	-16(%rbp), %eax
	movl	%eax, -48(%rbp)
	movl	-48(%rbp), %eax
	movl	%eax, -24(%rbp)
	leaq	.LCint_fmt(%rip), %rdi
	movl	-24(%rbp), %esi
	xor	%eax, %eax
	call	printf
	addq	$32, %rsp
	leave
	.cfi_def_cfa   7, 8
	.cfi_endproc
	ret
.LFE0:
	.size	main, .-main
