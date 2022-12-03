    .text
    .align 4
    .global _start
    .type   _start, @function
_start:
    ori t0, s0, -1
    ori t1, s0, 0x7ff
    ori t2, s0, 0x703
    ori t3, s0, 0
    ecall
