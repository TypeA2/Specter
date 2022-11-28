    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Just normal immediates
    li t0, 5
    li t1, 1234
    li t2, 0x7fa
    li t3, -1
    li t4, -0x7fa
    li t5, -1234

    # Ensure XLEN >32 works
    addi t6, t6, 1

    # Positive overflow into negative
    addi s0, s0, 128

    # 2 large numbers
    addi s2, s2, 0x7fa

    # Negative + negative
    addi s3, s3, -1111

    # Move some stuff
    mv s4, t1
    mv s5, t2
    mv s6, t4
    mv s7, t5
    mv s8, t6
    mv s9, s2
    mv s10, s3
    ecall
