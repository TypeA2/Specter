    .text
    .align 4
    .global _exit
    .type   _exit, @function
_exit:
    # a0 should already be set
    li a7, 93
    ecall
