#include <stdint.h>
#include <stdlib.h>

#define CARTRIDGE_STATUS (volatile uint32_t *)0x4000001C
void u32_print(uint32_t);
int main() {
  for (int i = 0;; ++i) {
    u32_print(i);
    if (*CARTRIDGE_STATUS & 1) {
      ((void (*)(void))((*CARTRIDGE_STATUS) & 0xFFFFFFFC))();
    }
  }
}
