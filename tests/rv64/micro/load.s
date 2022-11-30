    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Zero offset
    lb t0, 0(s0) 
    lbu t1, 0(s0)

    lh t2, 0(s0)
    lhu t3, 0(s0)

    lw t4, 0(s0)
    lwu t5, 0(s0)

    ld t6, 0(s0)

    # 1 dword offset
    lb s2, 8(s0)
    lbu s3, 8(s0)

    lh s4, 8(s0)
    lhu s5, 8(s0)

    lw s6, 8(s0)
    lwu s7, 8(s0)

    ld s8, 8(s0)

    # Negative offset
    lb s9, -8(s1)
    lbu s10, -8(s1)
    
    lh s11, -8(s1)
    lhu a1, -8(s1)

    lw a2, -8(s1)
    lwu a3, -8(s1)

    ld a4, -8(s1)

    ecall

    .data
    .align 4
    .type data, @object
data:
    .quad 0xc0ffee11deadbeef
    .quad 0x126d126d126d126d
