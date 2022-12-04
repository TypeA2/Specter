    .text
    .align 4
foo:
    li t1, 124 # executed
    c.beqz s1, end # does not jump
    li t2, 125 # executed
    c.beqz a1, end # jumps
    li t2, -1125 # not executed
    ecall

    .global _start
    .type   _start, @function
    .align 4
_start:
    li t0, 123      # executed
    c.beqz s0, foo  # jumps
    li t0, -1123 # not executed
    ecall
end:
    li t3, 126 # executed
    ecall
