#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BG_W 512
#define BG_H 288
#define BG_SIZE 147456
uint32_t get_ticks();
uint32_t uint_print(uint32_t);
uint32_t set_mode(uint32_t);
uint32_t get_controller();
uint32_t set_timer_callback(void (*callback)(), uint32_t ms);
uint32_t set_video_callback(void (*callback)(void *), void *arg);
uint32_t set_pixel_bg_data(uint32_t index, uint8_t *);
uint32_t set_pixel_bg_controls(uint32_t index, uint32_t controls);
uint32_t set_bg_palette(uint32_t, uint32_t *);

int main() {
  uint32_t bg_index = 0;
  uint32_t palette_index = 1;
  set_pixel_bg_controls(63, (bg_index << 29) | (3 << 22) | (287 << 12) |
                                (511 << 2) | palette_index);

  uint32_t palette[256];
  for (int32_t i = 0; i < 256; ++i) {
    palette[i] = 0xffff0000 + i;
  }
  set_bg_palette(palette_index, palette);

  uint8_t bg_data[BG_SIZE];
  for (int32_t i = 0; i < BG_SIZE; ++i) {
    bg_data[i] = i % 256;
  }
  set_pixel_bg_data(bg_index, bg_data);

  for (int i = 0;; i = (i + 1) & 63) {
    set_mode(0b111);
    set_pixel_bg_controls(63, (bg_index << 29) | (3 << 22) | (287 << 12) |
                                  (511 << 2) | palette_index);
    for (int32_t j = 0; j < BG_SIZE; ++j) {
      bg_data[j] = (j + i) % 256;
    }
    set_pixel_bg_data(bg_index, bg_data);
  }

  return 0;
}
