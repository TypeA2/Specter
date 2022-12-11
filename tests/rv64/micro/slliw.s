    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    slliw s0, t0, 1
    slliw s1, t0, 31
    slliw s2, t1, 0
    slliw s3, t1, 6
    slliw s4, t1, 28
    slliw s5, t1, 22
    slliw s6, t2, 14
    slliw s7, t2, 23
    ecall
