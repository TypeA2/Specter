    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    andi t0, s0, -1
    andi t1, s0, 0x7ff
    andi t2, s0, 0x703
    andi t3, s0, 0
    ecall
