# =========================
# math_exp.asm — реализация:
#   exp_series(y) = e^y  (редукция аргумента по ln 2 + ряд Тейлора)
#   inv_exp(x)   = e^(?x) = 1/exp(x)
#
# Протокол:
#   вход/выход double — через fa0; целые параметры — через a0..a2.
# Регистры:
#   Используются временные f-регистры (f1..f7), x-регистры t0, a0..a2.
# Сохранность:
#   exp_series сохраняет ra на стеке.
#   inv_exp тоже сохраняет/восстанавливает ra (так корректно по ABI).
# =========================

    .data
LN2:    .double 0.69314718055994530942   # ln(2)
TWO:    .double 2.0
HALF:   .double 0.5
ONE:    .double 1.0

    .text
    .globl exp_series
    .globl inv_exp

# fa0 = y  ? fa0 = e^y
exp_series:
    addi sp, sp, -16
    sw   ra, 12(sp)                 # сохранить адрес возврата

    # f1 = ln2; f2 = y/ln2
    la     t0, LN2
    fld    f1, 0(t0)
    fdiv.d f2, fa0, f1

    # k = round(y/ln2) (RNE по умолчанию)
    fcvt.w.d a0, f2                 # a0 = k (int32)
    fcvt.d.w f3, a0                 # f3 = double(k)

    # r = y ? k*ln2
    fmul.d f3, f3, f1
    fsub.d f4, fa0, f3              # f4 = r

    # Инициализация суммы: sum=1, term=1
    la   t0, ONE
    fld  f5, 0(t0)                  # f5 = sum
    la   t0, ONE
    fld  f6, 0(t0)                  # f6 = term

    li a1, 1                        # n = 1
    li a2, 8                        # N_TERMS = 8 (запас по точности)

.L_taylor:
    bgt  a1, a2, .L_scale

    fmul.d f6, f6, f4               # term *= r
    fcvt.d.w f7, a1
    fdiv.d   f6, f6, f7             # term /= n
    fadd.d   f5, f5, f6             # sum  += term

    addi a1, a1, 1
    j .L_taylor

# Масштабирование: e^y = 2^k * e^r
.L_scale:
    bgez a0, .L_scale_pos

    # k < 0 ? делим |k| раз на 2
    neg a0, a0
    la   t0, HALF
    fld  f1, 0(t0)
.L_scale_neg:
    beqz a0, .L_scaled
    fmul.d f5, f5, f1
    addi a0, a0, -1
    j .L_scale_neg

.L_scale_pos:
    beqz a0, .L_scaled
    la   t0, TWO
    fld  f1, 0(t0)
.L_scale_pos_loop:
    beqz a0, .L_scaled
    fmul.d f5, f5, f1
    addi a0, a0, -1
    j .L_scale_pos_loop

.L_scaled:
    fmv.d fa0, f5                   # результат ? fa0

    lw   ra, 12(sp)                 # восстановить ra
    addi sp, sp, 16
    ret

# fa0 = x  ? fa0 = e^(?x) = 1/exp(x)
inv_exp:
    # Сохранить ra (адрес возврата в вызывающего)
    addi sp, sp, -16
    sw   ra, 12(sp)

    fneg.d fa0, fa0                 # fa0 = ?x
    call  exp_series                # посчитать e^(?x)

    # Вернуть ra и вернуться к вызывающему
    lw   ra, 12(sp)
    addi sp, sp, 16
    ret
