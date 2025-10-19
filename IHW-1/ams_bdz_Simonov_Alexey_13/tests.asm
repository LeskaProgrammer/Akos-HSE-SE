# ========================= 
# tests.asm — генератор тестовых данных и раннер
# Содержит процедуру tests (запускает 10 кейсов подряд),
# и 10 процедур store_test_dataX, каждая — заполняет A и array_A_size.
# После каждой загрузки данных вызывается main_part из program.asm.
# =========================

.include "program.asm"

.text	
tests:
	# Запустить все тесты последовательно с пустой строкой между выводами.
	addi sp sp -4
    sw ra (sp)
        		
	newline
	call store_test_data1
	call main_part
		
	newline
	call store_test_data2
	call main_part
		
	newline
	call store_test_data3
	call main_part
		
	newline
	call store_test_data4
	call main_part
		
	newline
	call store_test_data5
	call main_part

	newline
	call store_test_data6
	call main_part

	newline
	call store_test_data7
	call main_part

	newline
	call store_test_data8
	call main_part

	newline
	call store_test_data9
	call main_part

	newline
	call store_test_data10
	call main_part
		
	lw ra (sp)
    addi sp sp 4
    ret
	
store_test_data1:
	# A = [3, -1, 4, 1, 5], |A| = 5
	la t0 array_A
	li t1 3
	sw t1 (t0)
	li t1 -1
	sw t1 4(t0)
	li t1 4
	sw t1 8(t0)
	li t1 1
	sw t1 12(t0)
	li t1 5
	sw t1 16(t0)
	la t0 array_A_size
	li t1 5
	sw t1 (t0)
	ret
	
store_test_data2:
	# A = [7, 7, 7, 7, 7], |A| = 5
	la t0 array_A
	li t1 7
	sw t1 (t0)
	li t1 7
	sw t1 4(t0)
	li t1 7
	sw t1 8(t0)
	li t1 7
	sw t1 12(t0)
	li t1 7
	sw t1 16(t0)
	la t0 array_A_size
	li t1 5
	sw t1 (t0)
	ret
	
store_test_data3:
	# A = [2, 2, 2, 2, 1], |A| = 5
	la t0 array_A
	li t1 2
	sw t1 (t0)
	li t1 2
	sw t1 4(t0)
	li t1 2
	sw t1 8(t0)
	li t1 2
	sw t1 12(t0)
	li t1 1
	sw t1 16(t0)
	la t0 array_A_size
	li t1 5
	sw t1 (t0)
	ret
	
store_test_data4:
	# A = [-5, 0, -5, 10, -2, -5], |A| = 6
	la t0 array_A
	li t1 -5
	sw t1 (t0)
	li t1 0
	sw t1 4(t0)
	li t1 -5
	sw t1 8(t0)
	li t1 10
	sw t1 12(t0)
	li t1 -2
	sw t1 16(t0)
	li t1 -5
	sw t1 20(t0)
	la t0 array_A_size
	li t1 6
	sw t1 (t0)
	ret
	
store_test_data5:
	# A = [-42], |A| = 1
	la t0 array_A
	li t1 -42
	sw t1 (t0)
	la t0 array_A_size
	li t1 1
	sw t1 (t0)
	ret

store_test_data6:
	# A = [9, 8, 7, 6, 5, 4, 3, 2, 1, 0], |A| = 10
	la t0 array_A
	li t1 9
	sw t1 (t0)
	li t1 8
	sw t1 4(t0)
	li t1 7
	sw t1 8(t0)
	li t1 6
	sw t1 12(t0)
	li t1 5
	sw t1 16(t0)
	li t1 4
	sw t1 20(t0)
	li t1 3
	sw t1 24(t0)
	li t1 2
	sw t1 28(t0)
	li t1 1
	sw t1 32(t0)
	li t1 0
	sw t1 36(t0)
	la t0 array_A_size
	li t1 10
	sw t1 (t0)
	ret

store_test_data7:
	# A = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0], |A| = 10
	la t0 array_A
	li t1 0
	sw t1 (t0)
	li t1 0
	sw t1 4(t0)
	li t1 0
	sw t1 8(t0)
	li t1 0
	sw t1 12(t0)
	li t1 0
	sw t1 16(t0)
	li t1 0
	sw t1 20(t0)
	li t1 0
	sw t1 24(t0)
	li t1 0
	sw t1 28(t0)
	li t1 0
	sw t1 32(t0)
	li t1 0
	sw t1 36(t0)
	la t0 array_A_size
	li t1 10
	sw t1 (t0)
	ret

store_test_data8:
	# A = [5, -3, -3, -3, 2, -3, 9], |A| = 7
	la t0 array_A
	li t1 5
	sw t1 (t0)
	li t1 -3
	sw t1 4(t0)
	li t1 -3
	sw t1 8(t0)
	li t1 -3
	sw t1 12(t0)
	li t1 2
	sw t1 16(t0)
	li t1 -3
	sw t1 20(t0)
	li t1 9
	sw t1 24(t0)
	la t0 array_A_size
	li t1 7
	sw t1 (t0)
	ret

store_test_data9:
	# A = [1, 2, 3, 4, -1, -1, -1, 0], |A| = 8
	la t0 array_A
	li t1 1
	sw t1 (t0)
	li t1 2
	sw t1 4(t0)
	li t1 3
	sw t1 8(t0)
	li t1 4
	sw t1 12(t0)
	li t1 -1
	sw t1 16(t0)
	li t1 -1
	sw t1 20(t0)
	li t1 -1
	sw t1 24(t0)
	li t1 0
	sw t1 28(t0)
	la t0 array_A_size
	li t1 8
	sw t1 (t0)
	ret

store_test_data10:
	# A = [100, -100, 0, 100, -100, 50, -50, 0, 1, -1], |A| = 10
	la t0 array_A
	li t1 100
	sw t1 (t0)
	li t1 -100
	sw t1 4(t0)
	li t1 0
	sw t1 8(t0)
	li t1 100
	sw t1 12(t0)
	li t1 -100
	sw t1 16(t0)
	li t1 50
	sw t1 20(t0)
	li t1 -50
	sw t1 24(t0)
	li t1 0
	sw t1 28(t0)
	li t1 1
	sw t1 32(t0)
	li t1 -1
	sw t1 36(t0)
	la t0 array_A_size
	li t1 10
	sw t1 (t0)
	ret
