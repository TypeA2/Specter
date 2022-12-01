    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Normal add
    addiw s0, t0, 0x7fa

    # One side negative
    addiw s1, t0, -42

    # Both sides negative
    addiw s2, t1, -42

    # Overflow past 32 bits
    addiw s3, t2, 0x7fa

    # Overflow into negative
    addiw s4, t3, 0x7fa

    # Sign extend negative
    sext.w s5, t5

    # And positive
    sext.w s6, t6

    ecall
