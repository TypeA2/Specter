    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    slti t1, t0, 42
    slti t3, t2, 42
    slti t5, t4, 0x7ff
    slti s0, t6, 42
    slti s2, s1, -1234
    slti s4, s3, -2047
    ecall
