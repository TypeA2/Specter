    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    srliw s0, t0, 1
    srliw s1, t0, 31
    srliw s2, t1, 0
    srliw s3, t1, 6
    srliw s4, t1, 28
    srliw s5, t1, 22
    srliw s6, t2, 14
    srliw s7, t2, 23
    srliw s8, t2, 6
    ecall
