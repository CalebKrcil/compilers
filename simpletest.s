	.file	"simpletest.kt"
	.text
	.section	.rodata
	.align	8
.LC0:
	.string	""meow""
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
	subq	$112, %rsp
.L0:
	pushq	%rbp
	movl	0(%rbp), %eax
	movl	%eax, 0(%rbp)
	subq	$8, %rsp
	pushq	0(%rbp)
	call	*-48(%rbp)
	movq	%rax, -8(%rbp)
	addq	$16, %rsp
	movl	0(%rbp), %eax
	leave
	ret
.L1:
	pushq	%rbp
	movl	0(%rbp), %eax
	movl	%eax, 0(%rbp)
	subq	$24, %rsp
	movl	0(%rbp), %eax
	addl	-8(%rbp), %eax
	movl	%eax, -24(%rbp)
	movl	-24(%rbp), %eax
	movl	%eax, -16(%rbp)
	addq	$32, %rsp
	movl	0(%rbp), %eax
	leave
	ret
.L2:
	pushq	%rbp
	movl	0(%rbp), %eax
	movl	%eax, 0(%rbp)
	subq	$32, %rsp
	movl	$1, -32(%rbp)
	movl	-32(%rbp), %eax
	movl	%eax, 0(%rbp)
	movl	$2, -40(%rbp)
	movl	-40(%rbp), %eax
	movl	%eax, -8(%rbp)
	movl	0(%rbp), %eax
	imull	-8(%rbp), %eax
	movl	%eax, -48(%rbp)
	movl	-48(%rbp), %eax
	movl	%eax, -16(%rbp)
	pushq	0(%rbp)
	call	*-48(%rbp)
	movq	%rax, -56(%rbp)
	pushq	0(%rbp)
	call	*0(%rbp)
	movq	%rax, -64(%rbp)
	movl	$1, -72(%rbp)
	pushq	-72(%rbp)
	movl	$2, -80(%rbp)
	pushq	-80(%rbp)
	call	*-8(%rbp)
	movq	%rax, -88(%rbp)
	movl	-88(%rbp), %eax
	movl	%eax, -24(%rbp)
	pushq	0(%rbp)
	pushq	-8(%rbp)
	call	*-8(%rbp)
	movq	%rax, -96(%rbp)
	pushq	-96(%rbp)
	call	*-48(%rbp)
	movq	%rax, -104(%rbp)
	addq	$112, %rsp
	movl	0(%rbp), %eax
	leave
	ret
	movl	$0, %eax
	leave
	.cfi_def_cfa 7, 8
	ret
.LFE0:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.4.0) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
