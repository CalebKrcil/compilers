	.file	"simpletest.kt"
	.section	.rodata
	.align	8
.LC0:
	.string	""meow""
	.text
	.globl	main
	.type	main, @function
main:
.LFB0:
	.cfi_startproc
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
	movl	-0(%rbp), %edi
	call	.L48
	movl	%eax, -8(%rbp)
	addq	$16, %rsp
	movl	-0(%rbp), %eax
	leave
	.cfi_def_cfa 7, 8
	ret
.LFE0:
	.size	main, .-main
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
	movl	-0(%rbp), %eax
	leave
	.cfi_def_cfa 7, 8
	ret
.LFE0:
	.size	main, .-main
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
	leaq	-0(%rbp), %rdi
	call	.L48
	movl	%eax, -56(%rbp)
	leaq	-0(%rbp), %rdi
	call	.L0
	movl	%eax, -64(%rbp)
	movl	$1, -72(%rbp)
	movl	$2, -80(%rbp)
	movl	-72(%rbp), %edi
	movl	-80(%rbp), %esi
	call	.L8
	movl	%eax, -88(%rbp)
	movl	-88(%rbp), %eax
	movl	%eax, -24(%rbp)
	movl	-0(%rbp), %edi
	movl	-8(%rbp), %esi
	call	.L8
	movl	%eax, -96(%rbp)
	movl	-96(%rbp), %edi
	call	.L48
	movl	%eax, -104(%rbp)
	addq	$112, %rsp
	movl	-0(%rbp), %eax
	leave
	.cfi_def_cfa 7, 8
	ret
.LFE0:
	.size	main, .-main
