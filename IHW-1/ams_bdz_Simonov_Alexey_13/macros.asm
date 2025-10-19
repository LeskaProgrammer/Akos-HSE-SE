# =========================
# macros.asm � ����� ������, ��������� � ��������������� ������������
# ��������:
#   � ����������� ������ �������� A � B (�� 40 ���� = �� 10 ����� �� 4 �����);
#   � ��������� ������� ��� �������� ���������� � a0�a4;
#   � ���������:
#       check_len                 � �������� ����� �� [1;10];
#       read_array_lenght         � ���� ���������� ����� ������� A � ��������;
#       fill_array_from_console   � ���� ��������� ������� A;
#       show_array                � ������ ������� � ������;
#       get_array_min             � ����� �������� � �������;
#       get_len_of_array_without_elem � ����� ��� ��������� ��������;
#       fill_B_array              � ���������� B �� A, �������� �������.
# ��������:
#   t0�t6 � ���������, s0 � ����������� (������������ ��� ��������).
# ������: call <�����> ��������� ra, ���� ��� ������ ���� ���������.
# =========================

.include "io.asm"
.eqv max_array_size_in_bytes 40

.data 
    .align 2
    array_A: .space max_array_size_in_bytes   # ����� ��� ������ A (�� 10 ��������� int32)
    array_A_size: .word 0                     # ����������� ����� A (� ���������)
    array_B: .space max_array_size_in_bytes   # ����� ��� ������ B
    array_B_size: .word 0                     # ����������� ����� B

.macro array_data_to_a_registers(%array_label, %array_size_label, %array_name)
	# ������� ������ �������, ���� ����� � ������ ����� � a0, a1, a2 ��������������.
	# ������ ����� ������� �������� show_array / get_array_min / � �. �.
	.data 
		name: .asciz %array_name
	.text
		la a0 %array_label
		lw a1 %array_size_label
		la a2 name
.end_macro

.macro arrays_data_to_a_registers(%arrayA_label, %arrayA_size_label, %arrayB_label, %arrayB_size_label, %smallest)
	# ������� ��������� ���� �������� � �������� �������� � a0�a4 ��� fill_B_array.
	la a0 %arrayA_label
	lw a1 %arrayA_size_label
	la a2 %arrayB_label
	lw a3 %arrayB_size_label
	mv a4 %smallest
.end_macro

.macro set_first_to_min(%register1, %register2)
	# ��������� %register1 := min(%register1, %register2) ��� ��������� �� ���������.
	ble %register1 %register2 end_of_macros
	mv %register1 %register2
end_of_macros:	
.end_macro

.text
check_len: 
	# �������� ����� � a0 �� ��������� � [1;10].
	# �������: a0 = 0 (���������) ��� 1 (�����������).
	.eqv correct_length 0
   	.eqv incorrect_length 1
   	.eqv max_length 10
   	.eqv min_length 1
   		
   	mv t2 a0                 # t2 ? ����������� �����
   	li a0 correct_length     # �� ��������� � OK
   	li t0 min_length
   	li t1 max_length
   	bgt t2 t1 bad_length     # t2 > 10 ? �����
   	blt t2 t0 bad_length     # t2 < 1  ? �����
   	j end_of_func
bad_length:
   	li a0 incorrect_length   # ������� ������
end_of_func:
   	ret
   	
read_array_lenght:
	# ���������� ���� ����� ������� A � ��������� � �������� ��� ������.
    # �������: a0 = ���������� �����.
    # ������������� s0: ������ ��������� ����; ra �����������.
   	addi sp sp -8            # ������: ����� ��� ra � s0
   	sw ra 4(sp)
   	sw s0 (sp)
begin:
   	print_str("len A [1..10]: ")
   	read_int(s0)             # s0 ? ���� ������������
   	register_to_a0(s0)
   	call check_len           # a0 = 0 ���� OK, ����� 1
   	bnez a0 error            # ���� 1 ? ���������
   	mv a0 s0                 # ������� ���������� �����
   	j end
error:
   	print_str("Wrong length... ERROR")
   	li a7, 10     # ������ 10: ��������� ���������
	ecall

end:
   	lw ra 4(sp)              # ������
   	lw s0 (sp) 
   	addi sp sp 8
   	ret
    
fill_array_from_console:
	# ���� ��������� �������.
    # ���������: a0 � ����� ������ A; a1 � ����� (N).
    # ������. �������: ��������� N * 4 ���� �� ������ a0.
   	mv t0 a0                 # t0 � ����� �������� ��������
   	li t1 0                  # t1 � ������ i
   	mv t2 a1                 # t2 � N
fill_loop:
   	print_str("Input index ")
   	print_int_from_register(t1)
   	print_str(") ")
   	read_int(t3)             # t3 ? ��������
   	sw t3 (t0)               # A[i] = t3
   	addi t0 t0 4             # ++�����
   	addi t1 t1 1             # ++i
   	blt t1 t2 fill_loop
    ret
    
show_array:
	# ������ ������� � �������: "This is <name> array : [ x y z ]"
	# ���������: a0 � �����, a1 � �����, a2 � ����� null-������ c ������.
	mv t0 a0                  # ������� �����
   	li t1 0                   # �������
   	mv t2 a1                  # �����
   	print_str("")
   	print_from_adress(a2)
   	print_str(" arr: [")
   	beqz t2 end_of_showing    # ������ ������ ? ������ ������
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
    # ����� ������� � �������.
    # ���������: a0 � �����, a1 � �����.
    # �������: a0 � ����������� ������� (int32).
   	mv t0 a0                  # ����� ��������
   	li t1 0                   # i
   	mv t2 a1                  # N
   	.eqv max_int 2147483647
    li t3 max_int             # t3 = +? ��� ��������� �������
find_min_loop:
   	lw t4(t0)                 # t4 = A[i]
   	set_first_to_min(t3, t4)  # t3 = min(t3, t4)
   	addi t0 t0 4
   	addi t1 t1 1
   	blt t1 t2 find_min_loop
   	mv a0 t3                  # ������� �������
    ret
	
get_len_of_array_without_elem:
	# ����������, ������� ��������� ������� �� ����� ���������.
	# ���������: a0 � �����, a1 � �����, a3 � ������� ��� ����������.
	# �������: a0 � ���������� ? a3.
	mv t0 a0
   	li t1 0
   	mv t2 a1
   	mv t3 a3
   	li t4 0                   # ������� ����������
get_len_loop:
   	lw t5 (t0)
   	beq t5 t3 end_of_get_len_loop  # ���������� ������
   	addi t4 t4 1
end_of_get_len_loop:
   	addi t1 t1 1
   	addi t0 t0 4
   	blt t1 t2 get_len_loop
   	mv a0 t4
   	ret
    
fill_B_array:
	# ��������� ������ B ����� ���������� A, ����� ������������ (a4).
	# ���������:
	#   a0 � ����� A, a1 � ����� A,
	#   a2 � ����� B, a3 � ����� B,
	#   a4 � ����������� ������� A.
	# ��������������, ��� a3 == ���-�� ��������� A, �������� �� ��������.
	addi sp sp -4
   	sw s0 (sp)                # �������� s0, ����� ������������ ��� ��������
    
   	mv t0 a0                  # ����� � A
   	mv t1 a1                  # ����� A
   	li t2 0                   # ������ �� A
   	mv t3 a2                  # ����� � B (���� ������)
   	mv t4 a3                  # ����� B (����� ������)
	li t5 0                   # ������ �� B
	mv s0 a4                  # s0 = ����������� ��������
fill_B_loop:
	lw t6 (t0)                # t6 = A[i]
	beq s0 t6 end_of_fill_B_loop   # ���� ������� � ����������
	sw t6 (t3)                # ����� ����� � B[j]
	addi t3 t3 4
	addi t5 t5 1
end_of_fill_B_loop:
	addi t2 t2 1             # ++i
	addi t0 t0 4
	blt t5 t4 fill_B_loop    # ���� �� ������� ��� �������� B
	
	lw s0 (sp)
	addi sp sp 4
	ret
