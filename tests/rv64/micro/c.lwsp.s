    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    c.lwsp t0, 0(sp)
    c.lwsp t1, 4(sp)
    c.lwsp t2, 8(sp)
    c.lwsp t3, 12(sp)
    c.lwsp t4, 16(sp)
    c.lwsp t5, 20(sp)
    c.lwsp s0, 24(sp)
    c.lwsp s1, 28(sp)
    c.lwsp s2, 32(sp)
    c.lwsp s3, 36(sp)
    c.lwsp s4, 40(sp)
    c.lwsp s5, 44(sp)
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
