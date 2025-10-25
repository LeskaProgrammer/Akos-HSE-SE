        .text
        .globl main


start:  j   main

        .eqv LED_RIGHT,   0xFFFF0010
        .eqv LED_LEFT,    0xFFFF0011
        .eqv SEG_DP,      0x80
        .eqv DELAY_ITERS, 800000       

        .data
segLUT: .byte 0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07
        .byte 0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71
        .text

show_hex:
        andi t0, a0, 0xF
        la   t1, segLUT
        add  t1, t1, t0
        lbu  t2, 0(t1)
        srli t3, a0, 4
        beq  t3, x0, no_dot
        ori  t2, t2, SEG_DP
no_dot: sb   t2, 0(a1)
        ret


delay:
        li   t0, DELAY_ITERS
d1:     addi t0, t0, -1
        bnez t0, d1
        ret

main:
    
        li   t0, 0
        li   a1, LED_LEFT
        sb   t0, 0(a1)
        li   a1, LED_RIGHT
        sb   t0, 0(a1)

        li   s2, 0    

loop:
 
        mv   a0, s2
        li   a1, LED_LEFT
        jal  ra, show_hex
        jal  ra, delay

  
        mv   a0, s2
        li   a1, LED_RIGHT
        jal  ra, show_hex
        jal  ra, delay

        addi s2, s2, 1
        andi s2, s2, 0x1F
        j    loop
