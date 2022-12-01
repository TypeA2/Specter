    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Normal add
    addw s0, t0, t1

    # One side negative
    addw s1, t0, t2

    # Both sides negative
    addw s2, t2, t2

    # Overflow past 32 bits
    addw s3, t3, t3

    # Overflow into negative
    addw s4, t4, t4

    ecall
