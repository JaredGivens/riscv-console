#include <stdint.h>
#include <stdlib.h>

#define CARTRIDGE_STATUS (volatile uint32_t *)0x4000001C
void uint_print(uint32_t);
int main() {
  for (int i = 0;; ++i) {
    uint_print(i);
    if (*CARTRIDGE_STATUS & 1) {
      ((void (*)(void))((*CARTRIDGE_STATUS) & 0xFFFFFFFC))();
    }
  }
}

extern char _heap_base[];
extern char _stack[];

char *_sbrk(int numbytes) {
  static char *heap_ptr = NULL;
  char *base;

  if (heap_ptr == NULL) {
    heap_ptr = (char *)&_heap_base;
  }

  if ((heap_ptr + numbytes) <= _stack) {
    base = heap_ptr;
    heap_ptr += numbytes;
    return (base);
  } else {
    // errno = ENOMEM;
    return NULL;
  }
}
