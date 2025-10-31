# =========================
# tests.asm — автономный прогон inv_exp(x) = 1/exp(x).
# Назначение: собрать и запустить отдельно (без main.asm).
# =========================

    .include "macros.asm"

    .data
hdr:    .asciz "Automated tests for inv_exp(x) = 1/exp(x):\n"
fmt1:   .asciz "x = "
fmt2:   .asciz "  ->  1/exp(x) = "
sep:    .asciz "-----------------------------\n"
tests:  .double -10.0, -5.0, -1.0, -0.5, 0.0, 0.5, 1.0, 5.0, 10.0, 50.0, 100.0, 700.0
NTEST:  .word 12

    .text
    .globl main

main:
    print_label(hdr)

    lw t0, NTEST
    la t1, tests

.Lloop:
    beqz t0, .Ldone

    # x ? fa0
    fld fa0, 0(t1)
    print_label(fmt1)
    print_double_from_fa0
    newline

    # 1/exp(x)
    call inv_exp
    print_label(fmt2)
    print_double_from_fa0
    newline
    print_label(sep)

    addi t1, t1, 8
    addi t0, t0, -1
    j .Lloop

.Ldone:
    exit

# Автономность: включаем реализацию математики здесь же.
    .include "math_exp.asm"
