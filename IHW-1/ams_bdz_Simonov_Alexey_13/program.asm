# =========================
# program.asm � ��������� �����: ����/����� � ��������� ��������
# �������� ��� ������� ���������:
#   input     � ���������� ���� ������� A � ����� ��������� ���������;
#   main_part � ���� ���������: ������ A, ����� ��������, ���������� B, ������ B.
# ���������� �� ������ `sw a0 array_A_size t0` � ����� ��������� �� �������� ��������
# ��� RARS (��������� ��� sw src, off(base)). ����� ��� ������� ��� ���������,
# ��������� �� ���������� ������� �� �������. ����������� � ����� ���� �� �� �����.
# =========================

.include "macros.asm"

.data 
 	is_tester: .word 0   # ��������� ����� (� ������ ���� �� ������������)
 
.text
input:
    # ���� ����� A, ����� ����� ���������, ����� ������ main_part.
    addi sp sp -4
    sw ra (sp)
        
    call read_array_lenght              # a0 = ���������� �����
    sw a0 array_A_size t0               # (��. ���������� � ��������� �����)
        
    array_data_to_a_registers(array_A, array_A_size, "A")
    call fill_array_from_console        # ���������� A
        
    call main_part                      # �������� ������
        
    lw ra (sp)
    addi sp sp 4
    ret
        
main_part:
    # ���������:
    #   1) �������� A;
    #   2) ����� ������� � A � ��������� ��� � s0;
    #   3) ��������� ����� B = |{x in A : x != min}|;
    #   4) ��������� B;
    #   5) �������� B.
    addi sp sp -4
    sw ra (sp)
        
 	array_data_to_a_registers(array_A, array_A_size, "A")
 	call show_array                     # ������ A
 		  
	array_data_to_a_registers(array_A, array_A_size, "A")
    call get_array_min                  # a0 = min(A)
    mv s0 a0                            # s0 ������ �������
        
    array_data_to_a_registers(array_A, array_A_size, "A")
    mv a3 s0                            # a3 = ������� (�������� �������)
    call get_len_of_array_without_elem  # a0 = |A \ {min}|
    sw a0 array_B_size t0               # (��. ���������� � ��������� �����)
        
    arrays_data_to_a_registers(array_A, array_A_size, array_B, array_B_size, s0)
    call fill_B_array                   # ���������� B
        
    array_data_to_a_registers(array_B, array_B_size, "B")
    call show_array                     # ������ B
        	
    lw ra (sp)
    addi sp sp 4
    ret
