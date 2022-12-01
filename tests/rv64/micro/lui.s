    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Just normal immediates
    lui s0, 0xab
    lui s1, 0x12345
    lui s2, 0xfabcd
    ecall
