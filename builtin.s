	.file	"builtin.kt"
	.section	.rodata
	.align	8
.D0:
	.double	-4.000000
.D1:
	.double	2.000000
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
	subq	$176, %rsp
	movsd	.D0(%rip), %xmm0
	movsd	%xmm0, -88(%rbp)
	movsd	-88(%rbp), %xmm0
	movsd	%xmm0, -8(%rbp)
	movsd	.D1(%rip), %xmm0
	movsd	%xmm0, -96(%rbp)
	movsd	-96(%rbp), %xmm0
	movsd	%xmm0, -16(%rbp)
	movsd	-8(%rbp), %xmm0
	movapd	%xmm0, %xmm1
	movabsq	$0x7FFFFFFFFFFFFFFF, %rax
	movq	%rax, %xmm2
	andpd	%xmm2, %xmm1
	movsd	%xmm1, -104(%rbp)
	movsd	-104(%rbp), %xmm0
	movsd	%xmm0, -24(%rbp)
	movsd	-8(%rbp), %xmm0
	movsd	-16(%rbp), %xmm1
	maxsd	%xmm1, %xmm0
	movsd	%xmm0, -112(%rbp)
	movsd	-112(%rbp), %xmm0
	movsd	%xmm0, -32(%rbp)
	movsd	-8(%rbp), %xmm0
	movsd	-16(%rbp), %xmm1
	minsd	%xmm1, %xmm0
	movsd	%xmm0, -120(%rbp)
	movsd	-120(%rbp), %xmm0
	movsd	%xmm0, -40(%rbp)
	movsd	-8(%rbp), %xmm0
	movsd	-16(%rbp), %xmm1
	call	pow
	movsd	%xmm0, -128(%rbp)
	movsd	-128(%rbp), %xmm0
	movsd	%xmm0, -48(%rbp)
	movsd	-16(%rbp), %xmm0
	call	sin
	movsd	%xmm0, -136(%rbp)
	movsd	-136(%rbp), %xmm0
	movsd	%xmm0, -56(%rbp)
	movsd	-16(%rbp), %xmm0
	call	cos
	movsd	%xmm0, -144(%rbp)
	movsd	-144(%rbp), %xmm0
	movsd	%xmm0, -64(%rbp)
	movsd	-16(%rbp), %xmm0
	call	tan
	movsd	%xmm0, -152(%rbp)
	movsd	-152(%rbp), %xmm0
	movsd	%xmm0, -72(%rbp)
	movl	$0, %edi
	call	time
	movq	%rax, %rdi
	call	srand
	call	rand
	movl	%eax, -160(%rbp)
	movl	-160(%rbp), %eax
	movl	%eax, -80(%rbp)
	leaq	.LCint_fmt(%rip), %rdi
	movl	-80(%rbp), %esi
	xor	%eax, %eax
	call	printf
	leaq	.LCdouble_fmt(%rip), %rdi
	movsd	-24(%rbp), %xmm0
	movl	$1, %eax
	call	printf
	leaq	.LCdouble_fmt(%rip), %rdi
	movsd	-32(%rbp), %xmm0
	movl	$1, %eax
	call	printf
	leaq	.LCdouble_fmt(%rip), %rdi
	movsd	-40(%rbp), %xmm0
	movl	$1, %eax
	call	printf
	leaq	.LCdouble_fmt(%rip), %rdi
	movsd	-48(%rbp), %xmm0
	movl	$1, %eax
	call	printf
	leaq	.LCdouble_fmt(%rip), %rdi
	movsd	-56(%rbp), %xmm0
	movl	$1, %eax
	call	printf
	leaq	.LCdouble_fmt(%rip), %rdi
	movsd	-64(%rbp), %xmm0
	movl	$1, %eax
	call	printf
	leaq	.LCdouble_fmt(%rip), %rdi
	movsd	-72(%rbp), %xmm0
	movl	$1, %eax
	call	printf
	addq	$176, %rsp
	leave
	.cfi_def_cfa   7, 8
	.cfi_endproc
	ret
.LFE0:
	.size	main, .-main
