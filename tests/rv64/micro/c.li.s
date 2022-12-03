    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Just normal immediates
    c.li t0, -32
    c.li t1, -14
    c.li t2, -1
    c.li t3, 0
    c.li t4, 3
    c.li t5, 21
    c.li t6, 31
    ecall
