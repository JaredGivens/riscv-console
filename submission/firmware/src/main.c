#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CARTRIDGE_STATUS (volatile uint32_t *)0x4000001C
int main() {
  while (1) {
    if (*CARTRIDGE_STATUS & 1) {
      ((void (*)(void))((*CARTRIDGE_STATUS) & 0xFFFFFFFC))();
    }
  }
}
