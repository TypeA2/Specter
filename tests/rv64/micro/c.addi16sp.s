    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    mv sp, ra
    c.addi16sp sp, -512
    mv t0, sp

    mv sp, ra
    c.addi16sp sp, -320
    mv t1, sp

    mv sp, ra
    c.addi16sp sp, -16
    mv t2, sp

    mv sp, ra
    c.addi16sp sp, 16
    mv t3, sp

    mv sp, ra
    c.addi16sp sp, 336
    mv t4, sp

    mv sp, ra
    c.addi16sp sp, 496
    mv t5, sp

    ecall
