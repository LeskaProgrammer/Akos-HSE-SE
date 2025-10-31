# =========================
# main.asm вЂ” ГІГ®Г·ГЄГ  ГўГµГ®Г¤Г  ГЁ ГўГ»ГЎГ®Г° Г°ГҐГ¦ГЁГ¬Г  (ГІГҐГ±ГІГ» / Г¤ГЁГ Г«Г®ГЈГ®ГўГ»Г© ГўГўГ®Г¤)
# Г‚Г§Г ГЁГ¬Г®Г¤ГҐГ©Г±ГІГўГіГҐГІ Г± tests (ГЁГ§ tests.asm) ГЁ input/main_part (ГЁГ§ program.asm).
# Г‹Г®ГЈГЁГЄГ :
#   вЂў ГЇГҐГ·Г ГІГ ГҐГ¬ ГЇГ°ГЁГЈГ«Г ГёГҐГ­ГЁГҐ;
#   вЂў ГҐГ±Г«ГЁ ГЇГ®Г«ГјГ§Г®ГўГ ГІГҐГ«Гј ГўГўГ®Г¤ГЁГІ 0 ? Г§Г ГЇГіГ±ГЄГ ГҐГ¬ Г ГўГІГ®Г¬Г ГІГЁГ·ГҐГ±ГЄГЁГҐ ГІГҐГ±ГІГ»;
#   вЂў ГЁГ­Г Г·ГҐ вЂ” Г¤ГЁГ Г«Г®ГЈГ®ГўГ»Г© ГўГўГ®Г¤ Г¬Г Г±Г±ГЁГўГ  ГЁ Г®ГЎГ°Г ГЎГ®ГІГЄГ .
# =========================

.text
j main
.include "tests.asm"    # ГЏГ®Г¤ГІГїГЈГЁГўГ ГҐГ¬ ГІГҐГ±ГІГ®ГўГ»Г© Г¤Г°Г Г©ГўГҐГ° ГЁ ГўГ±Гѕ Г«Г®ГЈГЁГЄГі ГЁГ§ program.asm
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
