    .text
    .align 4
    .global _start
    .type   _start, @function
next:
    c.mv s0, t0
    c.mv s1, t1
    c.mv s2, t2
    ecall
