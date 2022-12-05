    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    or t0, s0, s1
    or t1, s0, s2
    or t2, s0, s3
    or t3, s0, s4
    ecall
