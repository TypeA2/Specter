    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    addi s1, s0, 30
    ecall
