	.file	"boolean.c"
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
	movl	$1, -20(%rbp)				; move 1 into rpb-20				c code: int a = 1
	movl	$0, -16(%rbp)				; move 0 into rbp-16				c code: int b = 2
	cmpl	$0, -20(%rbp)				; check if a == 0
	je	.L2								; if a == 0, jump to L2
	cmpl	$0, -16(%rbp)				; check if b == 0
	je	.L2								; if b == 0, jump to L2
	movl	$1, %eax					; move 0 into eax (never run for this code)
	jmp	.L3
.L2:
	movl	$0, %eax					; move 0 into eax (this is result)
.L3:
	movl	%eax, -12(%rbp)				; move eax into rbp-12				c code: c = a && b
	cmpl	$0, -20(%rbp)				; check if a == 0
	jne	.L4								; if a != 0, jump to L4 (this is taken)
	cmpl	$0, -16(%rbp)				; check if b == 0
	je	.L5								; if b == 0 jump to L5
.L4:
	movl	$1, %eax					; result = 1
	jmp	.L6								; jump to L6
.L5:
	movl	$0, %eax					; result = 0 (not executed in this case)
.L6:
	movl	%eax, -12(%rbp)				; move the result into rbp-12		c code: c = a || b
	cmpl	$0, -20(%rbp)				; check if a == 0
	sete	%al							; if a is equal to 0, set al to 0
	movzbl	%al, %eax					; extend and move al into eax
	movl	%eax, -12(%rbp)				; move the result into c 			c code: c = !a
	movl	$5, -8(%rbp)				; assign 5 to rbp-8					c code: int x = 5
	movl	$10, -4(%rbp)				; assign 10 to rbp-4				c code: int y = 10
	movl	-8(%rbp), %eax				; move x into eax
	cmpl	-4(%rbp), %eax				; compare y to x
	setl	%al							; set al to the result of x < y 0 if x is greater, 1 if less
	movzbl	%al, %eax					; extend and move al into eax
	movl	%eax, -12(%rbp)				; set c equal to the result			c code: c = (x < y)
	movl	-8(%rbp), %eax				; move x into eax
	cmpl	-4(%rbp), %eax				; compare y to x
	setg	%al							; set al to result, 1 if greater, 0 if not
	movzbl	%al, %eax					; extend and move result into eax
	movl	%eax, -12(%rbp)				; move result to c					c code: c = (x > y)
	movl	-8(%rbp), %eax				; move x into eax
	cmpl	-4(%rbp), %eax				; compare x and y
	sete	%al							; set al to 1 if equal, 0 if not
	movzbl	%al, %eax					; extend and move al into eax
	movl	%eax, -12(%rbp)				; move result into c				c code: c = (x == y)
	movl	-8(%rbp), %eax				; move x into eax
	cmpl	-4(%rbp), %eax				; compare x and y
	setne	%al							; set al to 1 if not equal, 0 if equal
	movzbl	%al, %eax					; extend and move al into eax
	movl	%eax, -12(%rbp)				; move result into c 				c code: c = (x != y)
	movl	-8(%rbp), %eax				; move x into eax
	cmpl	-4(%rbp), %eax				; compare x and y
	setle	%al							; set al to 1 if less than or equal, 0 if not
	movzbl	%al, %eax					; extend and move al into eax
	movl	%eax, -12(%rbp)				; move result into c 				c code: c = (x <= y)
	movl	-8(%rbp), %eax				; move x into eax
	cmpl	-4(%rbp), %eax				; compare x and y
	setge	%al							; set al to 1 if greater than or equal, 0 if not
	movzbl	%al, %eax					; extend and move al into eax
	movl	%eax, -12(%rbp)				; move result into c 				c code: c = (x >= y)
	movl	$0, %eax					; return 0
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	main, .-main
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
