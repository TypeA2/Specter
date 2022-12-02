    .text
    .align 4
    .global _start
    .type   _start, @function
before:
    li s1, 128  # executed
    c.jr t1     # executed
    li s1, 1128 # not executed
    ecall       # not executed
_start:
    li s0, 127  # executed
    c.jr t0     # executed
    li s0, 1127 # not executed
    ecall
next:
    li s2, 129  # executed
    ecall
