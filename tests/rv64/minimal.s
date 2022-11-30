    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    li a0, -42
    li a7, 93
    ecall
