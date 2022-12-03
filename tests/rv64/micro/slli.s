    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    slli s0, t0, 1
    slli s1, t0, 43
    slli s2, t1, 0
    slli s3, t1, 6
    slli s4, t1, 60
    slli s5, t1, 63
    slli s6, t2, 14
    slli s7, t2, 53
    ecall
