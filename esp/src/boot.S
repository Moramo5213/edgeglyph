.section .text.entry
.global _start

_start:
    /* Set up stack pointer */
    la sp, _stack_top

    /* Clear BSS */
    la t0, _bss_start
    la t1, _bss_end
bss_clear_loop:
    beq t0, t1, bss_clear_done
    sw zero, 0(t0)
    addi t0, t0, 4
    j bss_clear_loop

bss_clear_done:
    /* Call kernel_main */
    call kernel_main

    /* Hang if kernel_main returns */
1:  j 1b
