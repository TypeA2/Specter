    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    c.addi4spn a1, sp, 4
    c.addi4spn a2, sp, 124
    c.addi4spn a3, sp, 652
    c.addi4spn a4, sp, 1020
    ecall
