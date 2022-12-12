    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Normal
    divu s0, t1, t0

    # One side negative
    divu s1, t2, t0

    # Both sides negative
    divu s2, t2, t4

    # >32-bit
    divu s3, t3, t1
    divu s4, t4, t3

    ecall
