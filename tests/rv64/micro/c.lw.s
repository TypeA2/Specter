    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    c.lw s0, 0(s1)
    c.lw a1, 8(s1)
    c.lw a2, 16(s1)
    c.lw a3, 24(s1)
    c.lw a4, 32(s1)
    c.lw a5, 40(s1)

    c.mv t0, s0
    c.mv t1, a1
    c.mv t2, a2
    c.mv t3, a3
    c.mv t4, a4
    c.mv t5, a5
    ecall

    .data
    .align 4
    .type data, @object
data:
    .quad 15
    .quad -1234
    .quad 0x10000
    .quad 0x20008
    .quad -4539648138848387345
    .quad 0x126d126d126d126d
