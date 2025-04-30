	.file	"statements.c"
	.text
	.section	.rodata
.LC0:
	.string	"%d\n"                            ; string literal for printf
	.text
	.globl	main
	.type	main, @function
main:
.LFB0:
	.cfi_startproc
	endbr64
	pushq	%rbp                               ; push old base pointer
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp                         ; set new base pointer
	.cfi_def_cfa_register 6
	subq	$16, %rsp                          ; allocate 16 bytes for locals
	movl	$0, -4(%rbp)                       ; int i = 0
	jmp	.L2                                 ; jump to loop test
.L4:
	movl	-4(%rbp), %ecx                     ; move i into ecx
	movslq	%ecx, %rax                         ; sign-extend ecx into rax
	imulq	$1431655766, %rax, %rax             ; multiply by magic number for divide by 3
	shrq	$32, %rax                          ; shift right 32 bits
	movq	%rax, %rdx                         ; move result to rdx
	movl	%ecx, %eax                         ; move ecx into eax
	sarl	$31, %eax                          ; arithmetic shift right by 31
	subl	%eax, %edx                         ; subtract eax from edx
	movl	%edx, %eax                         ; move edx to eax
	addl	%eax, %eax                         ; eax = 2*eax
	addl	%edx, %eax                         ; eax = 3*eax
	subl	%eax, %ecx                         ; ecx = i - 3*(i/3)
	movl	%ecx, %edx                         ; move remainder into edx
	cmpl	$1, %edx                           ; compare remainder to 1
	jne	.L3                                 ; if not equal to 1, jump to L3
	movl	-4(%rbp), %eax                     ; move i into eax
	movl	%eax, %esi                         ; move i into printf argument
	leaq	.LC0(%rip), %rax                   ; load address of format string
	movq	%rax, %rdi                         ; move format string into rdi
	movl	$0, %eax                           ; clear eax for varargs
	call	printf@PLT                         ; call printf
.L3:
	addl	$1, -4(%rbp)                       ; i = i + 1
.L2:
	cmpl	$19, -4(%rbp)                      ; compare i to 19
	jle	.L4                                 ; if i <= 19, loop
	movl	$0, %eax                           ; return 0
	leave                                     ; restore frame pointer
	.cfi_def_cfa 7, 8
	ret                                       ; return
	.cfi_endproc
.LFE0:
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
