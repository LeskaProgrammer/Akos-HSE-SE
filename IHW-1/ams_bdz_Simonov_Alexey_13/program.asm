# =========================
# program.asm — «рабочая» часть: ввод/вывод и обработка массивов
# Содержит две внешние процедуры:
#   input     — диалоговый ввод массива A и вызов основного алгоритма;
#   main_part — сама обработка: печать A, поиск минимума, построение B, печать B.
# Примечание по строке `sw a0 array_A_size t0` — такой синтаксис не является валидным
# для RARS (ожидается вид sw src, off(base)). Здесь код сохранён без изменений,
# поскольку по требованию «ничего не менять». Исполняться в таком виде он не будет.
# =========================

.include "macros.asm"

.data 
 	is_tester: .word 0   # служебная метка (в данном коде не используется)
 
.text
input:
    # Ввод длины A, затем самих элементов, затем запуск main_part.
    addi sp sp -4
    sw ra (sp)
        
    call read_array_lenght              # a0 = корректная длина
    sw a0 array_A_size t0               # (см. примечание в заголовке файла)
        
    array_data_to_a_registers(array_A, array_A_size, "A")
    call fill_array_from_console        # заполнение A
        
    call main_part                      # основная логика
        
    lw ra (sp)
    addi sp sp 4
    ret
        
main_part:
    # Обработка:
    #   1) показать A;
    #   2) найти минимум в A и сохранить его в s0;
    #   3) посчитать длину B = |{x in A : x != min}|;
    #   4) заполнить B;
    #   5) показать B.
    addi sp sp -4
    sw ra (sp)
        
 	array_data_to_a_registers(array_A, array_A_size, "A")
 	call show_array                     # печать A
 		  
	array_data_to_a_registers(array_A, array_A_size, "A")
    call get_array_min                  # a0 = min(A)
    mv s0 a0                            # s0 хранит минимум
        
    array_data_to_a_registers(array_A, array_A_size, "A")
    mv a3 s0                            # a3 = минимум (аргумент функции)
    call get_len_of_array_without_elem  # a0 = |A \ {min}|
    sw a0 array_B_size t0               # (см. примечание в заголовке файла)
        
    arrays_data_to_a_registers(array_A, array_A_size, array_B, array_B_size, s0)
    call fill_B_array                   # построение B
        
    array_data_to_a_registers(array_B, array_B_size, "B")
    call show_array                     # печать B
        	
    lw ra (sp)
    addi sp sp 4
    ret
