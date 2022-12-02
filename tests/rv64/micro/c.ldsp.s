    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Store some values
    #sd s0, 0(sp)
    #sd s1, 8(sp)
    #sd s2, 16(sp)
    #sd s3, 24(sp)
    #sd s4, 32(sp)
    #sd s5, 40(sp)

    # And just load them, it's not that difficult
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
    .quad 15
    .quad -1234
    .quad 0x10000
    .quad 0x20008
    .quad -4539648138848387345
    .quad 0x126d126d126d126d
