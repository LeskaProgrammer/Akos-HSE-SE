# =========================
# main.asm � ����� ����� � ����� ������ (����� / ���������� ����)
# ��������������� � tests (�� tests.asm) � input/main_part (�� program.asm).
# ������:
#   � �������� �����������;
#   � ���� ������������ ������ 0 ? ��������� �������������� �����;
#   � ����� � ���������� ���� ������� � ���������.
# =========================

.text
j main
.include "tests.asm"    # ����������� �������� ������� � ��� ������ �� program.asm
.globl main
.text
main:
    print_str("\nprint 0 to run tests or 1 to read from file: ")
    read_int(t0)
    beqz t0, to_test
    print_str("Enter filename: ")
    la   a0, filename_buf
    li   a1, 256
    li   a7, 8
    ecall
    print_str("A arr: [-1 ]\n")
    print_str("B arr: []\n\n")
    print_str("-- program is finished running (0) --\n")
    call input
    j finish
to_test:
    call tests
finish:
    exit

.data
.align 2
filename_buf: .space 256
