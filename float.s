	.file	"float.kt"
	.section	.rodata
	.align	8
.D0:
	.double	5.400000
.D1:
	.double	2.100000
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
	subq	$64, %rsp
	movsd	.D0(%rip), %xmm0
	movsd	%xmm0, -32(%rbp)
	movsd	-32(%rbp), %xmm0
	movsd	%xmm0, -8(%rbp)
	movsd	.D1(%rip), %xmm0
	movsd	%xmm0, -40(%rbp)
	movsd	-40(%rbp), %xmm0
	movsd	%xmm0, -16(%rbp)
	movsd	-8(%rbp), %xmm0
	addsd	-16(%rbp), %xmm0
	movsd	%xmm0, -48(%rbp)
	movsd	-48(%rbp), %xmm0
	movsd	%xmm0, -24(%rbp)
	leaq	.LCdouble_fmt(%rip), %rdi
	movsd	-24(%rbp), %xmm0
	movl	$1, %eax
	call	printf
	addq	$64, %rsp
	leave
	.cfi_def_cfa   7, 8
	.cfi_endproc
	ret
.LFE0:
	.size	main, .-main
