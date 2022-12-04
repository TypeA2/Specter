    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Normal
    sub s0, t0, t1

    # One side negative
    sub s1, t0, t2

    # Both sides negative
    sub s2, t2, t2

    # Overflows
    sub s3, t3, t4
    sub s4, t4, t3

    ecall
