    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    c.andi a1, -1
    c.andi a2, -32
    c.andi a3, 14
    c.andi a4, 0
    ecall
