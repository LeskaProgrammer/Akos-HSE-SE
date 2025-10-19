# =========================
# main.asm — òî÷êà âõîäà è âûáîð ðåæèìà (òåñòû / äèàëîãîâûé ââîä)
# Âçàèìîäåéñòâóåò ñ tests (èç tests.asm) è input/main_part (èç program.asm).
# Ëîãèêà:
#   • ïå÷àòàåì ïðèãëàøåíèå;
#   • åñëè ïîëüçîâàòåëü ââîäèò 0 ? çàïóñêàåì àâòîìàòè÷åñêèå òåñòû;
#   • èíà÷å — äèàëîãîâûé ââîä ìàññèâà è îáðàáîòêà.
# =========================

.text
j main
.include "tests.asm"    # Ïîäòÿãèâàåì òåñòîâûé äðàéâåð è âñþ ëîãèêó èç program.asm
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
    call input
    j finish
to_test:
    call tests
finish:
    exit

.data
.align 2
filename_buf: .space 256
