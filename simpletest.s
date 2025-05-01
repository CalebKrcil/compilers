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
	subq	$40, %rsp
	movl	-40(%rbp), %edi
	call	println
	addq	$40, %rsp
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
	movl	-8(%rbp), %eax
	addl	-16(%rbp), %eax
	movl	%eax, -48(%rbp)
	movl	-48(%rbp), %eax
	movl	%eax, -24(%rbp)
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
	movl	%eax, -8(%rbp)
	movl	$2, -64(%rbp)
	movl	-64(%rbp), %eax
	movl	%eax, -16(%rbp)
	movl	-8(%rbp), %eax
	imull	-16(%rbp), %eax
	movl	%eax, -72(%rbp)
	movl	-72(%rbp), %eax
	movl	%eax, -24(%rbp)
	leaq	.LC0(%rip), %rdi
	call	println
	leaq	.LC0(%rip), %rdi
	call	silly
	movl	%eax, -80(%rbp)
	movl	$1, -88(%rbp)
	movl	$2, -96(%rbp)
	movl	-88(%rbp), %edi
	movl	-96(%rbp), %esi
	call	multiple_args
	movl	%eax, -104(%rbp)
	movl	-104(%rbp), %eax
	movl	%eax, -32(%rbp)
	movl	-8(%rbp), %edi
	movl	-16(%rbp), %esi
	call	multiple_args
	movl	%eax, -112(%rbp)
	movl	-112(%rbp), %edi
	call	println
	addq	$56, %rsp
	leave
	.cfi_def_cfa   7, 8
	.cfi_endproc
	ret
.LFE2:
	.size	main, .-main
