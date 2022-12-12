    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Normal add
    mul s0, t0, t1

    # One side negative
    mul s1, t0, t2

    # Both sides negative
    mul s2, t2, t2

    # >32-bit
    mul s3, t3, t3
    mul s4, t3, t4

    ecall
