# ==============================================================================
# ЗАДАНИЕ: Игра "Жизнь" (Game of Life) - FIXED
# ПЛАТФОРМА: RARS 1.6 (RISC-V)
# ==============================================================================

.data
    # === ВИДЕОПАМЯТЬ ===
    # 32x32 клетки * (16x16 пикселей) = 512x512 экран
    display:        .space 4096     # 1024 клетки * 4 байта

    # === БУФЕРЫ ===
    next_gen:       .space 1024     # Буфер для вычислений (0/1)
    file_buffer:    .space 2048     # Буфер для содержимого файла
    filename:       .space 64       # Имя файла

    # === ЦВЕТА ===
    COLOR_LIVE:     .word 0x0000FF00  # Зеленый
    COLOR_DEAD:     .word 0x00000000  # Черный
    
    # === СТРОКИ ===
    str_menu:       .asciz "\n--- Game of Life ---\n1. Console Input (X Y)\n2. File Input (Grid)\nSelect mode (1/2): "
    str_x:          .asciz "X (0-31, -1 to start): "
    str_y:          .asciz "Y (0-31): "
    str_file:       .asciz "Enter filename: "
    str_err:        .asciz "Error: File error (Not found or path issue).\n"
    str_start:      .asciz "Simulation started...\n"

.text
.globl main

main:
    # 1. Очистка экрана
    jal clear_screen

    # 2. Вывод меню
    li a7, 4
    la a0, str_menu
    ecall

    li a7, 5        # Ввод режима
    ecall
    li t0, 2
    beq a0, t0, mode_file

    # ============================================================
    # РЕЖИМ 1: КОНСОЛЬНЫЙ ВВОД
    # ============================================================
mode_console:
input_loop:
    # Ввод X
    li a7, 4
    la a0, str_x
    ecall
    li a7, 5
    ecall
    mv t0, a0
    
    li t1, -1
    beq t0, t1, run_game

    # Ввод Y
    li a7, 4
    la a0, str_y
    ecall
    li a7, 5
    ecall
    mv t1, a0

    # Рисуем точку
    mv a0, t0
    mv a1, t1
    jal draw_pixel
    j input_loop

    # ============================================================
    # РЕЖИМ 2: ВВОД ИЗ ФАЙЛА (ИСПРАВЛЕНО)
    # ============================================================
mode_file:
    # Запрос имени
    li a7, 4
    la a0, str_file
    ecall

    # Чтение строки имени
    li a7, 8
    la a0, filename
    li a1, 63
    ecall

    # --- FIX: Очистка от \n и \r ---
    la t0, filename
clean_loop:
    lb t1, 0(t0)
    beqz t1, try_open       # Конец строки (0) -> открываем
    
    li t2, 10               # Символ \n
    beq t1, t2, fix_char
    li t2, 13               # Символ \r (важно для Windows)
    beq t1, t2, fix_char
    
    addi t0, t0, 1
    j clean_loop

fix_char:
    sb zero, 0(t0)          # Ставим 0 (терминатор строки)
    # Сразу идем открывать, хвост нам не нужен

try_open:
    # Открытие файла (Syscall 1024)
    li a7, 1024
    la a0, filename
    li a1, 0        # 0 = Read Only
    ecall
    
    li t0, -1
    beq a0, t0, file_error  # Если -1, то ошибка
    mv s0, a0       # Сохраняем дескриптор файла

    # Чтение файла (Syscall 63)
    li a7, 63
    mv a0, s0
    la a1, file_buffer
    li a2, 2048
    ecall
    mv s1, a0       # Сохраняем кол-во байт

    # Закрытие файла (Syscall 57)
    li a7, 57
    mv a0, s0
    ecall

    # --- ПАРСЕР (Text -> Grid) ---
    la s2, file_buffer
    add s3, s2, s1  # Конец буфера
    li t0, 0        # cur_x
    li t1, 0        # cur_y

parse_loop:
    bge s2, s3, run_game
    lb t2, 0(s2)
    addi s2, s2, 1

    li t3, 10       # \n
    beq t2, t3, p_newline
    li t3, 13       # \r
    beq t2, t3, parse_loop # Игнорируем
    
    li t3, 49       # '1'
    beq t2, t3, p_live
    
    # Любой другой символ (0, пробел) -> пропускаем (X++)
    addi t0, t0, 1
    j parse_loop

p_live:
    # Рисуем пиксель
    mv a0, t0
    mv a1, t1
    
    # Сохраняем регистры
    addi sp, sp, -8
    sw t0, 0(sp)
    sw t1, 4(sp)
    jal draw_pixel
    lw t0, 0(sp)
    lw t1, 4(sp)
    addi sp, sp, 8
    
    addi t0, t0, 1
    j parse_loop

p_newline:
    li t0, 0        # X=0
    addi t1, t1, 1  # Y++
    li t3, 32
    bge t1, t3, run_game
    j parse_loop

file_error:
    li a7, 4
    la a0, str_err
    ecall
    li a7, 10       # Exit
    ecall

    # ============================================================
    # ИГРОВОЙ ЦИКЛ
    # ============================================================
run_game:
    li a7, 4
    la a0, str_start
    ecall

game_loop:
    # Задержка 100 мс
    li a7, 32
    li a0, 100
    ecall

    jal compute_next_gen
    jal update_grid
    j game_loop

# ------------------------------------------------------------------------------
# ФУНКЦИИ
# ------------------------------------------------------------------------------

draw_pixel:
    # Защита границ
    li t0, 31
    bgt a0, t0, dp_ret
    bgt a1, t0, dp_ret
    
    la t0, display
    li t1, 32
    mul t1, t1, a1
    add t1, t1, a0
    slli t1, t1, 2
    add t0, t0, t1
    lw t2, COLOR_LIVE
    sw t2, 0(t0)
dp_ret:
    ret

clear_screen:
    la t0, display
    li t1, 1024
    lw t2, COLOR_DEAD
cl_loop:
    sw t2, 0(t0)
    addi t0, t0, 4
    addi t1, t1, -1
    bnez t1, cl_loop
    ret

compute_next_gen:
    addi sp, sp, -4
    sw ra, 0(sp)
    li s0, 0        # Y
loop_y:
    li t0, 32
    bge s0, t0, comp_done
    li s1, 0        # X
loop_x:
    li t0, 32
    bge s1, t0, comp_nx

    mv a0, s1
    mv a1, s0
    jal count_neighbors
    mv t2, a0       # Neighbors

    li t3, 32
    mul t3, t3, s0
    add t3, t3, s1
    slli t3, t3, 2
    la t4, display
    add t4, t4, t3
    lw t5, 0(t4)
    
    lw t6, COLOR_LIVE
    li t4, 0

    beq t5, t6, cell_alive
    
    li t6, 3
    beq t2, t6, mk_alive
    j save_st

cell_alive:
    li t6, 2
    beq t2, t6, mk_alive
    li t6, 3
    beq t2, t6, mk_alive
    j save_st

mk_alive:
    li t4, 1

save_st:
    li t3, 32
    mul t3, t3, s0
    add t3, t3, s1
    la t5, next_gen
    add t5, t5, t3
    sb t4, 0(t5)

    addi s1, s1, 1
    j loop_x

comp_nx:
    addi s0, s0, 1
    j loop_y
comp_done:
    lw ra, 0(sp)
    addi sp, sp, 4
    ret

count_neighbors:
    mv t0, a0
    mv t1, a1
    li t2, 0
    li t3, -1
cn_dy:
    li t4, 1
    bgt t3, t4, cn_ret
    li t5, -1
cn_dx:
    li t6, 1
    bgt t5, t6, cn_ny
    
    or t6, t3, t5
    beqz t6, cn_nx

    add t6, t0, t5
    andi t6, t6, 31
    add a2, t1, t3
    andi a2, a2, 31
    
    li a3, 32
    mul a3, a3, a2
    add a3, a3, t6
    slli a3, a3, 2
    la a4, display
    add a4, a4, a3
    lw a5, 0(a4)
    lw a6, COLOR_LIVE
    bne a5, a6, cn_nx
    addi t2, t2, 1

cn_nx:
    addi t5, t5, 1
    j cn_dx
cn_ny:
    addi t3, t3, 1
    j cn_dy
cn_ret:
    mv a0, t2
    ret

update_grid:
    la t0, display
    la t1, next_gen
    li t2, 1024
    lw t3, COLOR_LIVE
    lw t4, COLOR_DEAD
upd_l:
    lb t5, 0(t1)
    addi t1, t1, 1
    bnez t5, u_green
    sw t4, 0(t0)
    j u_next
u_green:
    sw t3, 0(t0)
u_next:
    addi t0, t0, 4
    addi t2, t2, -1
    bnez t2, upd_l
    ret