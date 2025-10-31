# =========================
# macros.asm — макросы для RARS (RISC-V)
# Назначение: единый I/O для int/double/строк + служебные обёртки.
# Важно: без локальных числовых меток внутри .macro (совместимо с RARS).
# =========================

# Печать null-terminated строки по МЕТКЕ в .data
.macro print_label (%lbl)
    la a0, %lbl
    li a7, 4          # print_string
    ecall
.end_macro

# Печать одиночного символа (ASCII код в аргументе)
.macro print_char (%ch)
    li a0, %ch
    li a7, 11         # print_char
    ecall
.end_macro

# Перевод строки
.macro newline
    li a0, 10         # '\n'
    li a7, 11
    ecall
.end_macro

# Печать int из произвольного регистра
.macro print_int_from_register (%reg)
    mv a0, %reg
    li a7, 1          # print_int
    ecall
.end_macro

# Ввод int в указанный регистр
.macro read_int (%reg)
    li a7, 5          # read_int
    ecall
    mv %reg, a0
.end_macro

# Печать double (значение ожидается в fa0)
.macro print_double_from_fa0
    li a7, 3          # print_double
    ecall
.end_macro

# Ввод double в fa0
.macro read_double_to_fa0
    li a7, 7          # read_double
    ecall
.end_macro

# Копировать произвольный x-регистр в a0
.macro register_to_a0 (%reg)
    mv a0, %reg
.end_macro

# Завершение программы
.macro exit
    li a7, 10         # exit
    ecall
.end_macro

# Обёртка-макрос для вычисления 1/exp(x):
#   вход:  fa0 = x (double)
#   выход: fa0 = 1/exp(x)
.macro invexp_fa0
    call inv_exp
.end_macro


