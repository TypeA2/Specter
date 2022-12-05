    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    # Stays positive
    c.mv a1, t0
    c.mv a2, t1
    c.subw a1, a2
    c.mv s0, a1

    # Becomes negative
    c.mv a1, t1
    c.mv a2, t0
    c.subw a1, a2
    c.mv s1, a1

    # Subtract negative from positive
    c.mv a1, t0
    c.mv a2, t2
    c.subw a1, a2
    c.mv s2, a1

    # Subtract negative from negative
    c.mv a1, t2
    c.mv a2, t2
    c.subw a1, a2
    c.mv s3, a1

    # Overflow into negative, so become positive
    c.mv a1, t4
    c.mv a2, t3
    c.subw a1, a2
    c.mv s4, a1

    # Overflow into positive, so become negative
    c.mv a1, t3
    c.mv a2, t4
    c.subw a1, a2
    c.mv s5, a1

    # Negate positive
    c.mv a1, a3 # a3 = 0
    c.mv a2, t5
    c.subw a1, a2
    c.mv s6, a1

    # Negate negative
    c.mv a1, a3 # a3 = 0
    c.mv a2, t6
    c.subw a1, a2
    c.mv s7, a1

    ecall
