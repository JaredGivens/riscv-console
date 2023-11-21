#include <stdint.h>

extern uint8_t _data[];
extern uint8_t _sdata[];
extern uint8_t _edata[];
extern uint8_t _data_source[];
extern uint8_t _bss[];
extern uint8_t _ebss[];

void init(void) {
  // set bss to zero
  for (uint8_t *i = _bss; i < _ebss; ++i) {
    *i = 0;
  }
  // copy data rom to data ram
  for (uint32_t i = 0; i < (_edata - _data); ++i) {
    _data[i] = _data_source[i];
  }
}
