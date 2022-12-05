    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Normal add
    c.mv a1, t0
    c.mv a2, t1
    c.addw a1, a2
    c.mv s0, a1

    # One side negative
    c.mv a1, t0
    c.mv a2, t2
    c.addw a1, a2
    c.mv s1, a1

    # Both sides negative
    c.mv a1, t2
    c.mv a2, t2
    c.addw a1, a2
    c.mv s2, a1

    # Overflow past 32 bits
    c.mv a1, t3
    c.mv a2, t3
    c.addw a1, a2
    c.mv s3, a1

    # Overflow into negative
    c.mv a1, t4
    c.mv a2, t4
    c.addw a1, a2
    c.mv s4, a1

    ecall
