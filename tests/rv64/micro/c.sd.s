    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    c.sd s1, 0(s0)
    c.sd a1, 8(s0)
    c.sd a2, 16(s0)
    c.sd a3, 24(s0)
    c.sd a4, 32(s0)
    c.sd a5, 40(s0)

    ld t0, 0(s0)
    ld t1, 8(s0)
    ld t2, 16(s0)
    ld t3, 24(s0)
    ld t4, 32(s0)
    ld t5, 40(s0)

    ecall

    .data
    .align 4
    .type data, @object
data:
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0
