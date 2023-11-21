.section .text, "ax"

.global get_ticks, uint_print, set_mode, get_controller
get_ticks:
    li a5, 0
    ecall
uint_print:
    li a5, 1
    ecall
set_mode:
    li a5, 2
    ecall
get_controller:
    li a5, 3
    ecall

.global get_pixel_bg_data, set_pixel_bg_controls, get_bg_palette
get_pixel_bg_data:
    li a5, 4
    ecall
set_pixel_bg_controls:
    li a5, 5
    ecall
get_bg_palette:
    li a5, 6
    ecall
