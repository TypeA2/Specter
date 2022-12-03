    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    c.slli s0, 1
    c.slli s1, 43
    c.slli s2, 6
    c.slli s3, 60
    c.slli s4, 63
    c.slli s5, 14
    c.slli s6, 53
    ecall
