    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    sraiw s0, t0, 1
    sraiw s1, t0, 31
    sraiw s2, t1, 0
    sraiw s3, t1, 6
    sraiw s4, t1, 28
    sraiw s5, t1, 22
    sraiw s6, t2, 14
    sraiw s7, t2, 23
    sraiw s8, t2, 6
    ecall
