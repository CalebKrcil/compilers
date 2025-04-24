	.file	"multiplicative.c"
	.text
	.section	.rodata
	.align 8											; align such that data items are put in addresses that are mutliple of 8
.LC2:
	.string	"Multiplication of doubles: %f * %f = %f\n"
	.text
	.globl	main
	.type	main, @function
main:
.LFB0:
	.cfi_startproc										; prologue until subq
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$48, %rsp									; allocate 48 bytes for local vars
	movl	$5, -36(%rbp)								; assign 5 to rbp-36					c code: a = 5
	movl	$3, -32(%rbp)								; assign 3 to rbp-32					c code: b = 3
	movsd	.LC0(%rip), %xmm0							; move binary encoding of 2.5 to xmm0
	movsd	%xmm0, -24(%rbp)							; assign 2.5 to rbp-24 (8 bytes from 32 because of c)	c code: x = 2.5
	movsd	.LC1(%rip), %xmm0							; move binary encoding of 4.0 to xmm0
	movsd	%xmm0, -16(%rbp)							; assign 4.0 to rbp-16					c code: y = 4.0
	movl	-36(%rbp), %eax								; move a or 5 into eax
	imull	-32(%rbp), %eax								; multiply a and b
	movl	%eax, -28(%rbp)								; assign the result to rbp-28			c code: c = a * b
	movsd	-24(%rbp), %xmm0							; move x into xmm0
	mulsd	-16(%rbp), %xmm0							; multiply x and y
	movsd	%xmm0, -8(%rbp)								; assign result to rbp-8				c code: z = x * y
	movsd	-8(%rbp), %xmm1								; put z in xmm1
	movsd	-16(%rbp), %xmm0							; put y in xmm0
	movq	-24(%rbp), %rax								; put x in rax
	movapd	%xmm1, %xmm2								; move z to xmm2
	movapd	%xmm0, %xmm1								; move y to xmm1
	movq	%rax, %xmm0									; move x to xmm0
	leaq	.LC2(%rip), %rax							; prepare string
	movq	%rax, %rdi									; move string to rdi
	movl	$3, %eax									; set eax to 3 for printing 3 floats
	call	printf@PLT									; call printf							c code: printf("Multiplication of doubles: %f * %f = %f\n", x, y, z);
	movl	-36(%rbp), %eax								; move a into eax
	cltd												; extend into edx:eax
	idivl	-32(%rbp)									; divide a by b, eax is result, edx is remainder
	movl	%eax, -28(%rbp)								; assign result to c					c code: c = a / b
	movsd	-24(%rbp), %xmm0							; move x into xmm0
	divsd	-16(%rbp), %xmm0							; divide x by y
	movsd	%xmm0, -8(%rbp)								; set z equal to the result				c code: z = x / y
	movl	-36(%rbp), %eax								; move a into eax
	cltd												; extend
	idivl	-32(%rbp)									; divide a by b
	movl	%edx, -28(%rbp)								; assign the remainder to call			c code: c = a % b
	movl	$0, %eax									; return 0								c code: return 0
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	main, .-main
	.section	.rodata
	.align 8
.LC0:
	.long	0
	.long	1074003968									; encoding of 2.5
	.align 8
.LC1:
	.long	0
	.long	1074790400									; encoding of 4.0
	.ident	"GCC: (Ubuntu 11.4.0-2ubuntu1~20.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
