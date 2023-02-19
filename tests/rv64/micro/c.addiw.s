    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Normal add
    c.addiw s0, 26

    # One side negative
    c.addiw s1, -26

    # Both sides negative
    c.addiw s2, -26

    # Overflow past 32 bits
    c.addiw s3, 26

    # Overflow into negative
    c.addiw s4, 26

    ecall
