    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    xor t0, s0, s1
    xor t1, s0, s2
    xor t2, s0, s3
    xor t3, s0, s4
    ecall
