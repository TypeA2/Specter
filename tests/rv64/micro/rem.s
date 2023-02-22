    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Normal
    rem s0, t1, t0

    # One side negative
    rem s1, t0, t2

    # Both sides negative
    rem s2, t2, t5

    # >32-bit
    rem s3, t3, t1
    rem s4, t3, t4

    ecall
