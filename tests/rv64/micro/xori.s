    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    not t0, s0
    xori t1, s0, 0x7ff
    xori t2, s0, 0x703
    xori t3, s0, 0
    ecall
