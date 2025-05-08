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
	.globl	use_double
	.type	use_double, @function
use_double:
.LFB0:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$48, %rsp
	movsd	%xmm0, -8(%rbp)
	movsd	%xmm1, -16(%rbp)
	movsd	-8(%rbp), %xmm0
	addsd	-16(%rbp), %xmm0
	movsd	%xmm0, -32(%rbp)
	movsd	-32(%rbp), %xmm0
	movsd	%xmm0, -24(%rbp)
	addq	$48, %rsp
	leave
	.cfi_def_cfa   7, 8
	.cfi_endproc
	ret
.LFE0:
	.size	use_double, .-use_double
	.globl	use_ints
	.type	use_ints, @function
use_ints:
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
	.size	use_ints, .-use_ints
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
	movsd	.D0(%rip), %xmm0
	movsd	%xmm0, -40(%rbp)
	movsd	-40(%rbp), %xmm0
	movsd	%xmm0, -8(%rbp)
	movsd	.D1(%rip), %xmm0
	movsd	%xmm0, -48(%rbp)
	movsd	-48(%rbp), %xmm0
	movsd	%xmm0, -16(%rbp)
	movsd	-8(%rbp), %xmm0
	movsd	-16(%rbp), %xmm1
	call	use_double
	movsd	%xmm0, -56(%rbp)
	movsd	-56(%rbp), %xmm0
	movsd	%xmm0, -24(%rbp)
	leaq	.LCdouble_fmt(%rip), %rdi
	movsd	-24(%rbp), %xmm0
	movl	$1, %eax
	call	printf
	movsd	-8(%rbp), %xmm0
	movsd	-16(%rbp), %xmm1
	call	use_double
	movsd	%xmm0, -64(%rbp)
	leaq	.LCdouble_fmt(%rip), %rdi
	movsd	-64(%rbp), %xmm0
	movl	$1, %eax
	call	printf
	movl	$1, -72(%rbp)
	movl	$2, -80(%rbp)
	movl	-72(%rbp), %edi
	movl	-80(%rbp), %esi
	call	use_ints
	movl	%eax, -88(%rbp)
	movl	-88(%rbp), %eax
	movl	%eax, -32(%rbp)
	leaq	.LCint_fmt(%rip), %rdi
	movl	-32(%rbp), %esi
	xor	%eax, %eax
	call	printf
	movl	$2, -96(%rbp)
	movl	$30, -104(%rbp)
	movl	-96(%rbp), %edi
	movl	-104(%rbp), %esi
	call	use_ints
	movl	%eax, -112(%rbp)
	leaq	.LCint_fmt(%rip), %rdi
	movl	-112(%rbp), %esi
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
