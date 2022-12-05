    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    c.mv a1, t0
    c.srli a1, 1
    c.mv s0, a1

    c.mv a1, t0
    c.srli a1, 43
    c.mv s1, a1

    c.mv a1, t1
    c.srli a1, 6
    c.mv s2, a1

    c.mv a1, t1
    c.srli a1, 38
    c.mv s3, a1

    c.mv a1, t1
    c.srli a1, 63
    c.mv s4, a1

    c.mv a1, t2
    c.srli a1, 14
    c.mv s5, a1

    c.mv a1, t2
    c.srli a1, 53
    c.mv s6, a1

    c.mv a1, t2
    c.srli a1, 6
    c.mv s7, a1
    ecall
