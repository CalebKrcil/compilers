	.file	"simpletest.kt"
	.section	.rodata
	.align	8
.LC0:
	.string	""meow""
	.text
	.globl	silly
	.type	silly, @function
silly:
.LFB0:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movl	-32(%rbp), %edi
	call	println
	movl	%eax, -40(%rbp)
	movl	-0(%rbp), %eax
	addq	$32, %rsp
	leave
	.cfi_def_cfa   7, 8
	.cfi_endproc
	ret
.LFE0:
	.size	silly, .-silly
	.globl	multiple_args
	.type	multiple_args, @function
multiple_args:
.LFB1:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$48, %rsp
	movl	0(%rbp), %eax
	addl	-8(%rbp), %eax
	movl	%eax, -48(%rbp)
	movl	-48(%rbp), %eax
	movl	%eax, -16(%rbp)
	movl	-0(%rbp), %eax
	addq	$48, %rsp
	leave
	.cfi_def_cfa   7, 8
	.cfi_endproc
	ret
.LFE1:
	.size	multiple_args, .-multiple_args
	.globl	main
	.type	main, @function
main:
.LFB2:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$56, %rsp
	movl	$1, -56(%rbp)
	movl	-56(%rbp), %eax
	movl	%eax, 0(%rbp)
	movl	$2, -64(%rbp)
	movl	-64(%rbp), %eax
	movl	%eax, -8(%rbp)
	movl	0(%rbp), %eax
	imull	-8(%rbp), %eax
	movl	%eax, -72(%rbp)
	movl	-72(%rbp), %eax
	movl	%eax, -16(%rbp)
	leaq	-0(%rbp), %rdi
	call	println
	movl	%eax, -80(%rbp)
	leaq	-0(%rbp), %rdi
	call	silly
	movl	%eax, -88(%rbp)
	movl	$1, -96(%rbp)
	movl	$2, -104(%rbp)
	movl	-96(%rbp), %edi
	movl	-104(%rbp), %esi
	call	multiple_args
	movl	%eax, -112(%rbp)
	movl	-112(%rbp), %eax
	movl	%eax, -24(%rbp)
	movl	-0(%rbp), %edi
	movl	-8(%rbp), %esi
	call	multiple_args
	movl	%eax, -120(%rbp)
	movl	-120(%rbp), %edi
	call	println
	movl	%eax, -128(%rbp)
	movl	-0(%rbp), %eax
	addq	$56, %rsp
	leave
	.cfi_def_cfa   7, 8
	.cfi_endproc
	ret
.LFE2:
	.size	main, .-main
