    .text
    .align 4
foo:
    li t0, 1234  # executed
    c.j next     # executed
    li t0, -1234 # not executed
    ecall

    .global _start
    .type   _start, @function
    .align 4
_start:
    c.j foo     # executed
    li t1, 42   # not executed
    ecall       # not executed
next:
    li t2, 0x55 # executed
    c.j bar     # executed
    li t2, 0x44 # not executed
    ecall       # not executed
bar:
    li t3, 128  # executed
    ecall       # executed
