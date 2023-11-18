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

.global set_timer_callback, set_video_callback, set_pixel_bg_data
set_timer_callback:
    li a5, 4
    ecall
set_video_callback:
    li a5, 5
    ecall
set_pixel_bg_data:
    li a5, 6
    ecall

.global set_pixel_bg_controls, set_bg_palette
set_pixel_bg_controls:
    li a5, 7
    ecall
set_bg_palette:
    li a5, 8
    ecall
