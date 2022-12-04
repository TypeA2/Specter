    .text
    .align 4
foo:
    li t1, 124 # executed
    bltu s2, s3, end # does not jump
    li t2, 125 # executed
    bltu s4, s5, end # jumps
    li t2, -1125 # not executed
    ecall

    .global _start
    .type   _start, @function
    .align 4
_start:
    li t0, 123      # executed
    bltu s0, s1, foo # jumps
    li t0, -1123 # not executed
    ecall
end:
    li t3, 126 # executed
    ecall
