    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Just normal immediates
    auipc s0, 0xab
    auipc s1, 0x12345
    auipc s2, 0xfabcd
    ecall
