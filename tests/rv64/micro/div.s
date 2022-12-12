    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Normal
    div s0, t1, t0

    # One side negative
    div s1, t0, t2

    # Both sides negative
    div s2, t2, s1

    # >32-bit
    div s3, t3, t1
    div s4, t3, t4

    ecall
