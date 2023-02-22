    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Normal
    remu s0, t1, t0

    # One side negative
    remu s1, t2, t0

    # Both sides negative
    remu s2, t5, t2

    # >32-bit
    remu s3, t3, t1
    remu s4, t4, t3

    ecall
