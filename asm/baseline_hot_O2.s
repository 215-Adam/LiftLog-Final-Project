	.file	"baseline_hot.c"
	.intel_syntax noprefix
	.text
	.p2align 4
	.globl	compute_stats
	.def	compute_stats;	.scl	2;	.type	32;	.endef
compute_stats:
	test	rcx, rcx
	je	.L24
	push	r14
	push	r13
	push	r12
	mov	r12, rdx
	push	rbp
	mov	ebp, r8d
	push	rdi
	push	rsi
	mov	rsi, rcx
	push	rbx
	sub	rsp, 32
	.p2align 4,,10
	.p2align 3
.L2:
	movsx	rax, DWORD PTR [rsi]
	test	eax, eax
	js	.L3
	mov	rdx, rax
	mov	r13, QWORD PTR 8[rsi]
	sal	rdx, 6
	add	rax, rdx
	lea	rdi, [r12+rax*8]
	mov	r14, QWORD PTR [rdi]
	test	r14, r14
	je	.L4
	mov	rbx, r14
	jmp	.L6
	.p2align 4,,10
	.p2align 3
.L28:
	mov	rbx, QWORD PTR 40[rbx]
	test	rbx, rbx
	je	.L4
.L6:
	mov	rcx, QWORD PTR [rbx]
	mov	rdx, r13
	call	strcmp
	test	eax, eax
	jne	.L28
	mov	eax, DWORD PTR 32[rbx]
	movsd	xmm4, QWORD PTR 8[rbx]
	movsd	xmm5, QWORD PTR 16[rbx]
	movsd	xmm3, QWORD PTR 24[rbx]
	add	eax, 1
.L13:
	pxor	xmm0, xmm0
	movsd	xmm2, QWORD PTR 16[rsi]
	cvtsi2sd	xmm0, DWORD PTR 24[rsi]
	movapd	xmm1, xmm0
	divsd	xmm1, QWORD PTR .LC1[rip]
	addsd	xmm1, QWORD PTR .LC2[rip]
	mulsd	xmm0, xmm2
	mulsd	xmm1, xmm2
	comisd	xmm1, xmm4
	jbe	.L7
	movsd	QWORD PTR 8[rbx], xmm1
.L7:
	comisd	xmm2, xmm5
	jbe	.L9
	movsd	QWORD PTR 16[rbx], xmm2
.L9:
	addsd	xmm3, xmm0
	mov	edx, DWORD PTR 28[rsi]
	mov	DWORD PTR 32[rbx], eax
	xor	eax, eax
	sub	edx, ebp
	movsd	QWORD PTR 24[rbx], xmm3
	cmp	edx, -6
	jl	.L11
	movsx	rax, edx
	imul	rax, rax, -1840700269
	shr	rax, 32
	add	eax, edx
	sar	edx, 31
	sar	eax, 2
	sub	eax, edx
	mov	edx, 63
	cmp	eax, 63
	cmovg	eax, edx
.L11:
	cdqe
	addsd	xmm0, QWORD PTR 8[rdi+rax*8]
	movsd	QWORD PTR 8[rdi+rax*8], xmm0
.L3:
	mov	rsi, QWORD PTR 32[rsi]
	test	rsi, rsi
	jne	.L2
	add	rsp, 32
	pop	rbx
	pop	rsi
	pop	rdi
	pop	rbp
	pop	r12
	pop	r13
	pop	r14
	ret
	.p2align 4,,10
	.p2align 3
.L4:
	mov	ecx, 48
	call	malloc
	mov	rcx, r13
	mov	rbx, rax
	call	strdup
	pxor	xmm3, xmm3
	mov	QWORD PTR [rdi], rbx
	mov	QWORD PTR [rbx], rax
	movapd	xmm5, xmm3
	mov	eax, 1
	movapd	xmm4, xmm3
	mov	QWORD PTR 8[rbx], 0x000000000
	mov	QWORD PTR 16[rbx], 0x000000000
	mov	QWORD PTR 24[rbx], 0x000000000
	mov	DWORD PTR 32[rbx], 0
	mov	QWORD PTR 40[rbx], r14
	jmp	.L13
.L24:
	ret
	.section .rdata,"dr"
	.align 8
.LC1:
	.long	0
	.long	1077805056
	.align 8
.LC2:
	.long	0
	.long	1072693248
	.def	strcmp;	.scl	2;	.type	32;	.endef
	.def	malloc;	.scl	2;	.type	32;	.endef
	.def	strdup;	.scl	2;	.type	32;	.endef
