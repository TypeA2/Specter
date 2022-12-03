    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Normal add
    c.add s0, t1

    # One side negative
    c.add s1, t2

    # Both sides negative
    c.add s2, t2

    # Overflow past 32 bits
    c.add s3, t3

    # Overflow into negative
    c.add s4, t4

    ecall
