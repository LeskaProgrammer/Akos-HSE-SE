# =========================
# macros.asm — общие данные, константы и алгоритмические подпрограммы
# Содержит:
#   • статические буферы массивов A и B (по 40 байт = до 10 целых по 4 байта);
#   • служебные макросы для упаковки аргументов в a0–a4;
#   • процедуры:
#       check_len                 — проверка длины на [1;10];
#       read_array_lenght         — ввод корректной длины массива A с повтором;
#       fill_array_from_console   — ввод элементов массива A;
#       show_array                — печать массива с именем;
#       get_array_min             — поиск минимума в массиве;
#       get_len_of_array_without_elem — длина без заданного значения;
#       fill_B_array              — построение B из A, исключая минимум.
# Регистры:
#   t0–t6 — временные, s0 — сохраняемый (используется для минимума).
# Вызовы: call <метка> сохраняют ra, если это делает сама процедура.
# =========================

.include "io.asm"
.eqv max_array_size_in_bytes 40

.data 
    .align 2
    array_A: .space max_array_size_in_bytes   # буфер под массив A (до 10 элементов int32)
    array_A_size: .word 0                     # фактическая длина A (в элементах)
    array_B: .space max_array_size_in_bytes   # буфер под массив B
    array_B_size: .word 0                     # фактическая длина B

.macro array_data_to_a_registers(%array_label, %array_size_label, %array_name)
	# Уложить «адрес массива», «его длину» и «адрес имени» в a0, a1, a2 соответственно.
	# Удобно перед вызовом процедур show_array / get_array_min / и т. п.
	.data 
		name: .asciz %array_name
	.text
		la a0 %array_label
		lw a1 %array_size_label
		la a2 name
.end_macro

.macro arrays_data_to_a_registers(%arrayA_label, %arrayA_size_label, %arrayB_label, %arrayB_size_label, %smallest)
	# Уложить параметры двух массивов и значение минимума в a0–a4 для fill_B_array.
	la a0 %arrayA_label
	lw a1 %arrayA_size_label
	la a2 %arrayB_label
	lw a3 %arrayB_size_label
	mv a4 %smallest
.end_macro

.macro set_first_to_min(%register1, %register2)
	# Присвоить %register1 := min(%register1, %register2) без ветвлений на равенство.
	ble %register1 %register2 end_of_macros
	mv %register1 %register2
end_of_macros:	
.end_macro

.text
check_len: 
	# Проверка длины в a0 на попадание в [1;10].
	# Возврат: a0 = 0 (корректно) или 1 (некорректно).
	.eqv correct_length 0
   	.eqv incorrect_length 1
   	.eqv max_length 10
   	.eqv min_length 1
   		
   	mv t2 a0                 # t2 ? запрошенная длина
   	li a0 correct_length     # по умолчанию — OK
   	li t0 min_length
   	li t1 max_length
   	bgt t2 t1 bad_length     # t2 > 10 ? плохо
   	blt t2 t0 bad_length     # t2 < 1  ? плохо
   	j end_of_func
bad_length:
   	li a0 incorrect_length   # признак ошибки
end_of_func:
   	ret
   	
read_array_lenght:
	# Диалоговый ввод длины массива A с проверкой и повтором при ошибке.
    # Возврат: a0 = корректная длина.
    # Использование s0: хранит последний ввод; ra сохраняется.
   	addi sp sp -8            # пролог: место под ra и s0
   	sw ra 4(sp)
   	sw s0 (sp)
begin:
   	print_str("len A [1..10]: ")
   	read_int(s0)             # s0 ? ввод пользователя
   	register_to_a0(s0)
   	call check_len           # a0 = 0 если OK, иначе 1
   	bnez a0 error            # если 1 ? повторить
   	mv a0 s0                 # вернуть корректную длину
   	j end
error:
   	print_str("Wrong length... ERROR")
   	li a7, 10     # сервис 10: завершить программу
	ecall

end:
   	lw ra 4(sp)              # эпилог
   	lw s0 (sp) 
   	addi sp sp 8
   	ret
    
fill_array_from_console:
	# Ввод элементов массива.
    # Параметры: a0 — адрес начала A; a1 — длина (N).
    # Побочн. эффекты: заполняет N * 4 байт по адресу a0.
   	mv t0 a0                 # t0 — адрес текущего элемента
   	li t1 0                  # t1 — индекс i
   	mv t2 a1                 # t2 — N
fill_loop:
   	print_str("Input index ")
   	print_int_from_register(t1)
   	print_str(") ")
   	read_int(t3)             # t3 ? значение
   	sw t3 (t0)               # A[i] = t3
   	addi t0 t0 4             # ++адрес
   	addi t1 t1 1             # ++i
   	blt t1 t2 fill_loop
    ret
    
show_array:
	# Печать массива в формате: "This is <name> array : [ x y z ]"
	# Параметры: a0 — адрес, a1 — длина, a2 — адрес null-строки c именем.
	mv t0 a0                  # текущий адрес
   	li t1 0                   # счётчик
   	mv t2 a1                  # длина
   	print_str("")
   	print_from_adress(a2)
   	print_str(" arr: [")
   	beqz t2 end_of_showing    # пустой массив ? только скобки
show_loop:
   	lw t3 (t0)
   	print_int_from_register(t3)
   	print_char(' ')
   	addi t0 t0 4
   	addi t1 t1 1
   	blt t1 t2 show_loop
end_of_showing:
   	print_str("]\n")
   	ret 
    
get_array_min:
    # Найти минимум в массиве.
    # Параметры: a0 — адрес, a1 — длина.
    # Возврат: a0 — минимальный элемент (int32).
   	mv t0 a0                  # адрес текущего
   	li t1 0                   # i
   	mv t2 a1                  # N
   	.eqv max_int 2147483647
    li t3 max_int             # t3 = +? как стартовый минимум
find_min_loop:
   	lw t4(t0)                 # t4 = A[i]
   	set_first_to_min(t3, t4)  # t3 = min(t3, t4)
   	addi t0 t0 4
   	addi t1 t1 1
   	blt t1 t2 find_min_loop
   	mv a0 t3                  # вернуть минимум
    ret
	
get_len_of_array_without_elem:
	# Подсчитать, сколько элементов массива НЕ равны заданному.
	# Параметры: a0 — адрес, a1 — длина, a3 — элемент для исключения.
	# Возврат: a0 — количество ? a3.
	mv t0 a0
   	li t1 0
   	mv t2 a1
   	mv t3 a3
   	li t4 0                   # счётчик результата
get_len_loop:
   	lw t5 (t0)
   	beq t5 t3 end_of_get_len_loop  # пропускаем равные
   	addi t4 t4 1
end_of_get_len_loop:
   	addi t1 t1 1
   	addi t0 t0 4
   	blt t1 t2 get_len_loop
   	mv a0 t4
   	ret
    
fill_B_array:
	# Заполнить массив B всеми элементами A, КРОМЕ минимального (a4).
	# Параметры:
	#   a0 — адрес A, a1 — длина A,
	#   a2 — адрес B, a3 — длина B,
	#   a4 — минимальный элемент A.
	# Предполагается, что a3 == кол-ву элементов A, отличных от минимума.
	addi sp sp -4
   	sw s0 (sp)                # сохраним s0, будем использовать под «минимум»
    
   	mv t0 a0                  # адрес в A
   	mv t1 a1                  # длина A
   	li t2 0                   # индекс по A
   	mv t3 a2                  # адрес в B (куда писать)
   	mv t4 a3                  # длина B (лимит записи)
	li t5 0                   # индекс по B
	mv s0 a4                  # s0 = минимальное значение
fill_B_loop:
	lw t6 (t0)                # t6 = A[i]
	beq s0 t6 end_of_fill_B_loop   # если минимум — пропускаем
	sw t6 (t3)                # иначе пишем в B[j]
	addi t3 t3 4
	addi t5 t5 1
end_of_fill_B_loop:
	addi t2 t2 1             # ++i
	addi t0 t0 4
	blt t5 t4 fill_B_loop    # пока не набрали все элементы B
	
	lw s0 (sp)
	addi sp sp 4
	ret
