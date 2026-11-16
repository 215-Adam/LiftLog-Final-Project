	.file	"optimized_hot.c"
	.intel_syntax noprefix
	.text
	.p2align 4
	.globl	compute_stats
	.def	compute_stats;	.scl	2;	.type	32;	.endef
compute_stats:
	test	edx, edx
	jle	.L13
	lea	eax, -1[rdx]
	push	rsi
	movsd	xmm4, QWORD PTR .LC0[rip]
	mov	r11d, 63
	push	rbx
	sal	rax, 4
	movsx	rsi, DWORD PTR 56[rsp]
	movsd	xmm3, QWORD PTR .LC1[rip]
	lea	rbx, 16[rcx+rax]
	.p2align 4,,10
	.p2align 3
.L7:
	mov	r10d, DWORD PTR 4[rcx]
	movzx	edx, WORD PTR 12[rcx]
	pxor	xmm0, xmm0
	pxor	xmm2, xmm2
	cvtss2sd	xmm2, DWORD PTR [rcx]
	mov	rax, r10
	imul	rax, rsi
	add	rax, rdx
	movzx	edx, WORD PTR 14[rcx]
	sal	rax, 5
	cvtsi2sd	xmm0, edx
	add	rax, r8
	movapd	xmm1, xmm0
	divsd	xmm1, xmm4
	mulsd	xmm0, xmm2
	addsd	xmm1, xmm3
	mulsd	xmm1, xmm2
	comisd	xmm1, QWORD PTR [rax]
	jbe	.L3
	movsd	QWORD PTR [rax], xmm1
.L3:
	comisd	xmm2, QWORD PTR 8[rax]
	jbe	.L5
	movsd	QWORD PTR 8[rax], xmm2
.L5:
	movsd	xmm1, QWORD PTR 16[rax]
	add	DWORD PTR 24[rax], 1
	addsd	xmm1, xmm0
	movsd	QWORD PTR 16[rax], xmm1
	mov	eax, DWORD PTR 8[rcx]
	mov	rdx, rax
	imul	rax, rax, 613566757
	shr	rax, 32
	sub	edx, eax
	shr	edx
	add	eax, edx
	mov	edx, eax
	shr	edx, 2
	cmp	eax, 255
	cmova	rdx, r11
	sal	r10, 6
	add	rcx, 16
	add	r10, rdx
	lea	rax, [r9+r10*8]
	addsd	xmm0, QWORD PTR [rax]
	movsd	QWORD PTR [rax], xmm0
	cmp	rbx, rcx
	jne	.L7
	pop	rbx
	pop	rsi
	ret
	.p2align 4,,10
	.p2align 3
.L13:
	ret
	.section .rdata,"dr"
	.align 8
.LC0:
	.long	0
	.long	1077805056
	.align 8
.LC1:
	.long	0
	.long	1072693248
