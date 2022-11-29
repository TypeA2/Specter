    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    sltiu t1, t0, 42
    sltiu t3, t2, 42
    sltiu t5, t4, 0x7ff
    sltiu s0, t6, 42
    sltiu s2, s1, -1234
    sltiu s4, s3, -2047
    ecall
