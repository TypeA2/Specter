    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    srli s0, t0, 1
    srli s1, t0, 43
    srli s2, t1, 0
    srli s3, t1, 6
    srli s4, t1, 38
    srli s5, t1, 63
    srli s6, t2, 14
    srli s7, t2, 53
    srli s8, t2, 6
    ecall
