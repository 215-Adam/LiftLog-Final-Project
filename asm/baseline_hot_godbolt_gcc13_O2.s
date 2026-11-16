compute_stats:
        push    r15
        push    r14
        push    r13
        push    r12
        push    rbp
        push    rbx
        sub     rsp, 24
        mov     QWORD PTR [rsp+8], rdx
        test    rdi, rdi
        je      .L1
        mov     rbx, rdi
        mov     r15, rsi
.L2:
        movsx   rax, DWORD PTR [rbx]
        test    eax, eax
        js      .L3
        mov     rdx, rax
        mov     rbp, QWORD PTR [rbx+8]
        sal     rdx, 6
        add     rax, rdx
        lea     r12, [r15+rax*8]
        mov     r13, QWORD PTR [r12]
        test    r13, r13
        je      .L4
        mov     r14, r13
        jmp     .L6
.L26:
        mov     r14, QWORD PTR [r14+40]
        test    r14, r14
        je      .L4
.L6:
        mov     rdi, QWORD PTR [r14]
        mov     rsi, rbp
        call    strcmp
        test    eax, eax
        jne     .L26
        mov     eax, DWORD PTR [r14+32]
        movsd   xmm5, QWORD PTR [r14+8]
        movsd   xmm4, QWORD PTR [r14+16]
        movsd   xmm3, QWORD PTR [r14+24]
        add     eax, 1
.L13:
        pxor    xmm0, xmm0
        movsd   xmm2, QWORD PTR [rbx+16]
        cvtsi2sd        xmm0, DWORD PTR [rbx+24]
        movapd  xmm1, xmm0
        divsd   xmm1, QWORD PTR .LC1[rip]
        addsd   xmm1, QWORD PTR .LC2[rip]
        mulsd   xmm0, xmm2
        mulsd   xmm1, xmm2
        comisd  xmm1, xmm5
        jbe     .L7
        movsd   QWORD PTR [r14+8], xmm1
.L7:
        comisd  xmm2, xmm4
        jbe     .L9
        movsd   QWORD PTR [r14+16], xmm2
.L9:
        addsd   xmm3, xmm0
        mov     DWORD PTR [r14+32], eax
        mov     rcx, QWORD PTR [rbx+32]
        mov     rax, QWORD PTR [rsp+8]
        sub     rcx, rax
        movsd   QWORD PTR [r14+24], xmm3
        xor     eax, eax
        cmp     rcx, -6
        jl      .L11
        mov     eax, 63
        cmp     rcx, 447
        jg      .L11
        movabs  rax, 5270498306774157605
        imul    rcx
        sar     rcx, 63
        mov     rax, rdx
        sar     rax
        sub     eax, ecx
.L11:
        cdqe
        addsd   xmm0, QWORD PTR [r12+8+rax*8]
        movsd   QWORD PTR [r12+8+rax*8], xmm0
.L3:
        mov     rbx, QWORD PTR [rbx+40]
        test    rbx, rbx
        jne     .L2
.L1:
        add     rsp, 24
        pop     rbx
        pop     rbp
        pop     r12
        pop     r13
        pop     r14
        pop     r15
        ret
.L4:
        mov     edi, 48
        call    malloc
        mov     rdi, rbp
        mov     r14, rax
        call    strdup
        pxor    xmm3, xmm3
        pxor    xmm0, xmm0
        mov     QWORD PTR [r14+24], 0x000000000
        mov     QWORD PTR [r14], rax
        movapd  xmm4, xmm3
        mov     eax, 1
        movapd  xmm5, xmm3
        mov     DWORD PTR [r14+32], 0
        mov     QWORD PTR [r14+40], r13
        mov     QWORD PTR [r12], r14
        movups  XMMWORD PTR [r14+8], xmm0
        jmp     .L13
.LC1:
        .long   0
        .long   1077805056
.LC2:
        .long   0
        .long   1072693248
