    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    c.sw s1, 0(s0)
    c.sw a1, 4(s0)
    c.sw a2, 8(s0)
    c.sw a3, 12(s0)
    c.sw a4, 16(s0)
    c.sw a5, 24(s0)

    lw t0, 0(s0)
    lw t1, 4(s0)
    lw t2, 8(s0)
    lw t3, 12(s0)
    lw t4, 16(s0)
    lw t5, 24(s0)

    ecall

    .data
    .align 4
    .type data, @object
data:
    .quad 0
    .quad 0
    .quad 0
    .word 0
