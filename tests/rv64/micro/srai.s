    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    srai s0, t0, 1
    srai s1, t0, 43
    srai s2, t1, 0
    srai s3, t1, 6
    srai s4, t1, 38
    srai s5, t1, 63
    srai s6, t2, 14
    srai s7, t2, 53
    srai s8, t2, 6
    ecall
