#include <stdint.h>

#define CARTRIDGE_STATUS (volatile uint32_t *)0x4000001C
void printf(const char *, ...);
int main() {
  printf("polling for cartridge");
  for (int i = 0;; ++i) {
    if (*CARTRIDGE_STATUS & 1) {
      ((void (*)(void))((*CARTRIDGE_STATUS) & 0xFFFFFFFC))();
    }
  }
}
