	.file	"simpletest.kt"
	.section	.rodata
	.align	8
.LC0:
	.string	"meow\n"
.LCdouble_fmt:
	.string "%f\n"
.LCint_fmt:
	.string "%d\n"
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
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	xor	%eax, %eax
	call	printf
	addq	$16, %rsp
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
	movl	%edi, -8(%rbp)
	movl	%esi, -16(%rbp)
	movl	-8(%rbp), %eax
	addl	-16(%rbp), %eax
	movl	%eax, -32(%rbp)
	movl	-32(%rbp), %eax
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
	subq	$128, %rsp
	movl	$1, -40(%rbp)
	movl	-40(%rbp), %eax
	movl	%eax, -8(%rbp)
	movl	$2, -48(%rbp)
	movl	-48(%rbp), %eax
	movl	%eax, -16(%rbp)
	movl	-8(%rbp), %eax
	imull	-16(%rbp), %eax
	movl	%eax, -56(%rbp)
	movl	-56(%rbp), %eax
	movl	%eax, -24(%rbp)
	leaq	.LC0(%rip), %rdi
	xor	%eax, %eax
	call	printf
	leaq	.LC0(%rip), %rdi
	call	silly
	movl	%eax, -64(%rbp)
	movl	$1, -72(%rbp)
	movl	$2, -80(%rbp)
	movl	-72(%rbp), %edi
	movl	-80(%rbp), %esi
	call	multiple_args
	movl	%eax, -88(%rbp)
	movl	-88(%rbp), %eax
	movl	%eax, -32(%rbp)
	leaq	.LCint_fmt(%rip), %rdi
	movl	-32(%rbp), %esi
	xor	%eax, %eax
	call	printf
	movl	-32(%rbp), %eax
	movl	%eax, -96(%rbp)
	movl	-32(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -32(%rbp)
	leaq	.LCint_fmt(%rip), %rdi
	movl	-32(%rbp), %esi
	xor	%eax, %eax
	call	printf
	movl	-32(%rbp), %eax
	movl	%eax, -104(%rbp)
	movl	-32(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -32(%rbp)
	movl	-32(%rbp), %eax
	movl	%eax, -112(%rbp)
	movl	-32(%rbp), %eax
	subl	$1, %eax
	movl	%eax, -32(%rbp)
	leaq	.LCint_fmt(%rip), %rdi
	movl	-32(%rbp), %esi
	xor	%eax, %eax
	call	printf
	movl	-8(%rbp), %edi
	movl	-16(%rbp), %esi
	call	multiple_args
	movl	%eax, -120(%rbp)
	leaq	.LCint_fmt(%rip), %rdi
	movl	-120(%rbp), %esi
	xor	%eax, %eax
	call	printf
	addq	$128, %rsp
	leave
	.cfi_def_cfa   7, 8
	.cfi_endproc
	ret
.LFE2:
	.size	main, .-main
	.section .note.GNU-stack,"",@progbits
