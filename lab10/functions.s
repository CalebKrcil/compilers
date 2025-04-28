	.file	"functions.c"
	.text
	.section	.rodata
.LC0:
	.string	"%d is a multiple of 3\n"
	.text
	.globl	print_multiple_three
	.type	print_multiple_three, @function
print_multiple_three:
.LFB0:
	.cfi_startproc
	endbr64
	pushq	%rbp                               ; push old base pointer
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp                         ; set new base pointer
	.cfi_def_cfa_register 6
	subq	$16, %rsp                          ; allocate 16 bytes for locals
	movl	%edi, -4(%rbp)                     ; store argument x at -4(%rbp)
	movl	-4(%rbp), %ecx                     ; move x into ecx
	movslq	%ecx, %rax                         ; sign-extend ecx into rax
	imulq	$1431655766, %rax, %rax             ; multiply by magic number for divide by 3
	shrq	$32, %rax                          ; shift right 32 bits
	movq	%rax, %rdx                         ; move result to rdx
	movl	%ecx, %eax                         ; move ecx to eax
	sarl	$31, %eax                          ; arithmetic shift right by 31
	subl	%eax, %edx                         ; subtract eax from edx
	movl	%edx, %eax                         ; move edx to eax
	addl	%eax, %eax                         ; eax = 2*eax
	addl	%edx, %eax                         ; eax = 3*eax
	subl	%eax, %ecx                         ; ecx = x - 3*(x/3)
	movl	%ecx, %edx                         ; move remainder into edx
	testl	%edx, %edx                         ; test if remainder == 0
	jne	.L3                                 ; if not zero, jump to L3
	movl	-4(%rbp), %eax                     ; move x into eax
	movl	%eax, %esi                         ; move x into printf argument
	leaq	.LC0(%rip), %rax                   ; load address of format string
	movq	%rax, %rdi                         ; move format string into rdi
	movl	$0, %eax                           ; clear eax for varargs
	call	printf@PLT                         ; call printf
.L3:
	nop                                       ; no operation
	leave                                     ; restore frame pointer
	.cfi_def_cfa 7, 8
	ret                                       ; return
	.cfi_endproc
.LFE0:
	.size	print_multiple_three, .-print_multiple_three
	.globl	main
	.type	main, @function
main:
.LFB1:
	.cfi_startproc
	endbr64
	pushq	%rbp                               ; push old base pointer
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp                         ; set new base pointer
	.cfi_def_cfa_register 6
	subq	$16, %rsp                          ; allocate 16 bytes for locals
	movl	$0, -4(%rbp)                       ; int i = 0
	jmp	.L5                                 ; jump to loop condition
.L6:
	movl	-4(%rbp), %eax                     ; move i into eax
	movl	%eax, %edi                         ; move i into function argument
	call	print_multiple_three               ; call print_multiple_three(i)
	addl	$1, -4(%rbp)                       ; i = i + 1
.L5:
	cmpl	$9, -4(%rbp)                       ; compare i to 9
	jle	.L6                                 ; if i <= 9, loop
	movl	$0, %eax                           ; return 0
	leave                                     ; restore frame pointer
	.cfi_def_cfa 7, 8
	ret                                       ; return
	.cfi_endproc
.LFE1:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0"
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
