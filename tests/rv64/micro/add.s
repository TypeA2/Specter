    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Normal add
    add s0, t0, t1

    # One side negative
    add s1, t0, t2

    # Both sides negative
    add s2, t2, t2

    # Overflow past 32 bits
    add s3, t3, t3

    # Overflow into negative
    add s4, t4, t4

    ecall
