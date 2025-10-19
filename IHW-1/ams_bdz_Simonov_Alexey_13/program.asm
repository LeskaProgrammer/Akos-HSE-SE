# =========================
# program.asm Ч Ђрабоча€ї часть: ввод/вывод и обработка массивов
# —одержит две внешние процедуры:
#   input     Ч диалоговый ввод массива A и вызов основного алгоритма;
#   main_part Ч сама обработка: печать A, поиск минимума, построение B, печать B.
# ѕримечание по строке `sw a0 array_A_size t0` Ч такой синтаксис не €вл€етс€ валидным
# дл€ RARS (ожидаетс€ вид sw src, off(base)). «десь код сохранЄн без изменений,
# поскольку по требованию Ђничего не мен€тьї. »сполн€тьс€ в таком виде он не будет.
# =========================

.include "macros.asm"

.data 
 	is_tester: .word 0   # служебна€ метка (в данном коде не используетс€)
 
.text
input:
    # ¬вод длины A, затем самих элементов, затем запуск main_part.
    addi sp sp -4
    sw ra (sp)
        
    call read_array_lenght              # a0 = корректна€ длина
    sw a0 array_A_size t0               # (см. примечание в заголовке файла)
        
    array_data_to_a_registers(array_A, array_A_size, "A")
    call fill_array_from_console        # заполнение A
        
    call main_part                      # основна€ логика
        
    lw ra (sp)
    addi sp sp 4
    ret
        
main_part:
    # ќбработка:
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
