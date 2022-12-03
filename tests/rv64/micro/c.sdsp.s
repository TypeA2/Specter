    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    c.sdsp s0, 0(sp)
    c.sdsp s1, 8(sp)
    c.sdsp s2, 16(sp)
    c.sdsp s3, 24(sp)
    c.sdsp s4, 32(sp)
    c.sdsp s5, 40(sp)

    c.ldsp t0, 0(sp)
    c.ldsp t1, 8(sp)
    c.ldsp t2, 16(sp)
    c.ldsp t3, 24(sp)
    c.ldsp t4, 32(sp)
    c.ldsp t5, 40(sp)
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
