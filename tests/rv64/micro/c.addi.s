    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    c.nop
    c.addi t0, -32
    c.addi t1, -14
    c.addi t2, -1
    c.addi t3, 0
    c.addi t4, 3
    c.addi t5, 21
    c.addi t6, 31
    ecall
