# =========================
# main.asm — пользовательский интерфейс к 1/exp(x) (double)
# Режимы:
#   0 — интерактивный ввод x, печать 1/exp(x)
#   1 — автотесты по фиксированному набору x
# =========================

    .include "macros.asm"

    .data
mode_prompt: .asciz "Select mode: 0 - interactive, 1 - tests: "
prompt:      .asciz "Enter x (double): "
result_s:    .asciz "\nResult 1/exp(x) = "
warn_s:      .asciz "\n[Note] Very large |x| may overflow/underflow double.\n"
limit709:    .double 709.0

# Данные для тестового режима (печать списка x и соответствующих 1/exp(x))
hdr:    .asciz "Automated tests for inv_exp(x) = 1/exp(x):\n"
fmt1:   .asciz "x = "
fmt2:   .asciz "  ->  1/exp(x) = "
sep:    .asciz "-----------------------------\n"
tests:  .double -10.0, -5.0, -1.0, -0.5, 0.0, 0.5, 1.0, 5.0, 10.0, 50.0, 100.0, 700.0
NTEST:  .word 12

    .text
    .globl main

# Точка входа
main:
    # --- выбор режима ---
    print_label(mode_prompt)
    read_int(t0)                  # t0 = 0 или 1
    li t1, 1
    beq t0, t1, run_tests         # 1 ? режим автотестов

# =========================
# РЕЖИМ 0: интерактив
# =========================
interactive:
    print_label(prompt)
    read_double_to_fa0            # fa0 = x (double)

    # Мягкое предупреждение для |x| >= 709 (граница double)
    fneg.d   f1, fa0              # f1 = ?x
    fsgnjx.d f2, f1, f1           # f2 = |x|
    la       t0, limit709
    fld      f3, 0(t0)            # f3 = 709.0
    fle.d    t1, f3, f2           # t1 = (709.0 <= |x|)
    beqz     t1, .no_warn
    print_label(warn_s)
.no_warn:

    # Расчёт: вход fa0=x ? выход fa0=1/exp(x)
        # invexp_fa0
    call inv_exp


    # Вывод результата
    print_label(result_s)
    print_double_from_fa0
    newline
    exit

# =========================
# РЕЖИМ 1: автотесты
# =========================
run_tests:
    print_label(hdr)

    lw t2, NTEST                  # t2 = количество тестов
    la t3, tests                  # t3 ? массив double

.t_loop:
    beqz t2, .t_done

    # Подать x в fa0
    fld fa0, 0(t3)

    # Печать x
    print_label(fmt1)
    print_double_from_fa0
    newline

    # Расчёт и вывод
    call inv_exp
    print_label(fmt2)
    print_double_from_fa0
    newline
    print_label(sep)

    addi t3, t3, 8                # следующий double (8 байт)
    addi t2, t2, -1
    j .t_loop

.t_done:
    exit

# Подключаем реализацию математических подпрограмм ПОСЛЕ main.
    .include "math_exp.asm"
