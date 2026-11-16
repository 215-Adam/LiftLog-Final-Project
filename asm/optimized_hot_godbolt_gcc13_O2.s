compute_stats:
        mov     r9, rdx
        test    rsi, rsi
        jle     .L1
        sal     rsi, 4
        movsd   xmm4, QWORD PTR .LC0[rip]
        movsd   xmm3, QWORD PTR .LC1[rip]
        movsx   r8, r8d
        lea     rdx, [rdi+rsi]
.L3:
        mov     esi, DWORD PTR [rdi+4]
        movzx   r10d, WORD PTR [rdi+12]
        pxor    xmm2, xmm2
        pxor    xmm1, xmm1
        cvtss2sd        xmm1, DWORD PTR [rdi]
        mov     rax, rsi
        imul    rax, r8
        add     rax, r10
        movzx   r10d, WORD PTR [rdi+14]
        sal     rax, 5
        cvtsi2sd        xmm2, r10d
        add     rax, r9
        movupd  xmm5, XMMWORD PTR [rax]
        add     DWORD PTR [rax+24], 1
        mov     r10d, DWORD PTR [rdi+8]
        movapd  xmm0, xmm2
        divsd   xmm0, xmm4
        addsd   xmm0, xmm3
        mulsd   xmm0, xmm1
        unpcklpd        xmm0, xmm1
        maxpd   xmm0, xmm5
        mulsd   xmm1, xmm2
        movups  XMMWORD PTR [rax], xmm0
        movsd   xmm0, QWORD PTR [rax+16]
        addsd   xmm0, xmm1
        movsd   QWORD PTR [rax+16], xmm0
        mov     rax, r10
        imul    r10, r10, 613566757
        shr     r10, 32
        sub     eax, r10d
        shr     eax
        add     eax, r10d
        mov     r10d, 63
        shr     eax, 2
        cmp     eax, r10d
        cmova   rax, r10
        sal     rsi, 6
        add     rdi, 16
        add     rax, rsi
        lea     rax, [rcx+rax*8]
        addsd   xmm1, QWORD PTR [rax]
        movsd   QWORD PTR [rax], xmm1
        cmp     rdi, rdx
        jne     .L3
.L1:
        ret
.LC0:
        .long   0
        .long   1077805056
.LC1:
        .long   0
        .long   1072693248
