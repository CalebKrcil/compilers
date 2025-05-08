	.file	"arrays.kt"
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
	subq	$80, %rsp
	movl	$8, -16(%rbp)
	movl	-16(%rbp), %eax
	imull	$4, %eax
	movl	%eax, -24(%rbp)
	movl	-24(%rbp), %edi
	call	malloc
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	movq	%rax, -8(%rbp)
	movl	$0, -40(%rbp)
.L1:
	movl	-40(%rbp), %eax
	cmpl	-16(%rbp), %eax
	setge	%al
	movzbl	%al, %eax
	movl	%eax, -48(%rbp)
	cmpl	$0, -48(%rbp)
	jne	.L2
	movl	-40(%rbp), %eax
	imull	$4, %eax
	movl	%eax, -56(%rbp)
	movq	-8(%rbp), %rax
	movslq	-56(%rbp), %rcx
	addq	%rcx, %rax
	movq	%rax, -64(%rbp)
	movl	$0, -72(%rbp)
	movq	-64(%rbp), %rax
	movl	-72(%rbp), %eax
	movl	%eax, (%rax)
	movl	-40(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -40(%rbp)
	jmp	.L1
.L2:
	addq	$80, %rsp
	leave
	.cfi_def_cfa   7, 8
	.cfi_endproc
	ret
.LFE0:
	.size	main, .-main
	.section .note.GNU-stack,"",@progbits
