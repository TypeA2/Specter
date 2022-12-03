    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Just normal immediates
    c.lui s0, (1 << 20) - 32 # -32
    c.lui s1, (1 << 20) - 14 # -14
    c.lui s2, (1 << 20) - 1  # -1
    c.lui s3, 0x01
    c.lui s4, 0x0d
    c.lui s5, 0x1f
    ecall
