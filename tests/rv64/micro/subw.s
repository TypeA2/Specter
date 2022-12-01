    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Stays positive
    subw s0, t0, t1

    # Becomes negative
    subw s1, t1, t0

    # Subtract negative from positive
    subw s2, t0, t2

    # Subtract negative from negative
    subw s3, t2, t2

    # Overflow into negative, so become positive
    subw s4, t4, t3

    # Overflow into positive, so become negative
    subw s5, t3, t4

    # Negate positive
    negw s6, t5

    # Negate negative
    negw s7, t6

    ecall
