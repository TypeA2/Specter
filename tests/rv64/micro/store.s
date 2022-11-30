    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Zero offset
    sb s2, 0(s0)
    lb t0, 0(s0)

    sh s2, 0(s0)
    lh t1, 0(s0)

    sw s2, 0(s0)
    lw t2, 0(s0)

    sd s2, 0(s0)
    ld t3, 0(s0)

    # Some offset
    sb s2, 8(s0)
    lb t4, 8(s0)

    sh s2, 8(s0)
    lh t5, 8(s0)

    sw s2, 8(s0)
    lw t6, 8(s0)

    sd s2, 8(s0)
    ld s3, 8(s0)

    # Negative offset
    sb s2, -8(s1)
    lb s4, -8(s1)

    sh s2, -8(s1)
    lh s5, -8(s1)

    sw s2, -8(s1)
    lw s6, -8(s1)

    sd s2, -8(s1)
    ld s7, -8(s1)
    ecall

    .data
    .align 4
    .type data, @object
data:
    .quad 0
    .quad 0
