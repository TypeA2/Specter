    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    and t0, s0, s1
    and t1, s0, s2
    and t2, s0, s3
    and t3, s0, s4
    ecall
