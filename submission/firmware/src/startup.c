#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define MTIME_LOW (*((volatile uint32_t *)0x40000008))
#define MTIME_HIGH (*((volatile uint32_t *)0x4000000C))
#define MTIMECMP_LOW (*((volatile uint32_t *)0x40000010))
#define MTIMECMP_HIGH (*((volatile uint32_t *)0x40000014))
#define CONTROLLER (*((volatile uint32_t *)0x40000018))

#define INTERRUPT_ENABLE (*((volatile uint32_t *)0x40000000))
#define INTERRUPT_PENDING (*((volatile uint32_t *)0x40000004))

#define DMA1_SRC (*((volatile uint32_t *)0x40000020))
#define DMA1_DEST (*((volatile uint32_t *)0x40000024))
#define DMA1_SIZE (*((volatile uint32_t *)0x40000028))
#define DMA1_STATUS (*((volatile uint32_t *)0x4000002C))
#define DTA 0x40000000

#define MODE_CONTROL (*((volatile uint32_t *)0x500F6780))

volatile int ticks;
volatile uint32_t controller_status;

uint8_t *BG_BUFS = (void *)0x50000000;
uint32_t BG_BUF_SIZE = 0x24000;
volatile uint32_t *BG_CONTROLS = (volatile void *)0x500F5A00;
uint8_t *BG_PALETTES = (void *)0x500F0000;
uint32_t BG_PALETTE_SIZE = 0x400;
char *TEXT_DATA = (char *)0x500F4800;

extern uint8_t _data[];
extern uint8_t _sdata[];
extern uint8_t _edata[];
extern uint8_t _data_source[];
extern uint8_t _bss[];
extern uint8_t _ebss[];

void (*timer_callback)(void);
uint32_t timer_period;
uint64_t timer_last; // time when callback was last called

void (*video_callback)(void *);
void *video_arg;
void (*controller_callback)(void *, uint32_t);
void *controller_arg;

// Adapted from
// https://stackoverflow.com/questions/58947716/how-to-interact-with-risc-v-csrs-by-using-gcc-c-code
__attribute__((always_inline)) inline uint32_t csr_mstatus_read(void) {
  uint32_t result;
  asm volatile("csrr %0, mstatus" : "=r"(result));
  return result;
}

__attribute__((always_inline)) inline void csr_mstatus_write(uint32_t val) {
  asm volatile("csrw mstatus, %0" : : "r"(val));
}

__attribute__((always_inline)) inline void csr_write_mie(uint32_t val) {
  asm volatile("csrw mie, %0" : : "r"(val));
}

__attribute__((always_inline)) inline void csr_enable_interrupts(void) {
  asm volatile("csrsi mstatus, 0x8");
}

__attribute__((always_inline)) inline void csr_disable_interrupts(void) {
  asm volatile("csrci mstatus, 0x8");
}

void *buf_set(void *dest, uint32_t n, uint32_t size) {
  for (int32_t i = size - 1; i > -1; --i) {
    ((uint8_t *)dest)[i] = n;
  }
  return dest;
}

void *buf_cpy(void *dest, void *src, uint32_t size) {
  // DMA1_SRC = (uint32_t)src;
  // DMA1_DEST = (uint32_t)dest;
  // DMA1_SIZE = size;
  // DMA1_STATUS = DTA;
  // while (DMA1_STATUS & DTA) {
  // }
  for (int32_t i = size - 1; i > -1; --i) {
    ((uint8_t *)dest)[i] = ((uint8_t *)src)[i];
  }
  return dest;
}

void init(void) {
  // set bss to zero
  buf_set(_bss, 0, _ebss - _bss);
  // zero controls
  // might not be neccessary
  buf_set((void *)BG_CONTROLS, 0, 64);
  // copy data rom to data ram
  buf_cpy(_data, _data_source, _edata - _data);

  INTERRUPT_ENABLE = 0b10;
  MTIMECMP_LOW = 1;
  MTIMECMP_HIGH = 0;
}

void u32_print(uint32_t num) {
  if (MODE_CONTROL & 0b1) {
    return;
  }
  char str[64];
  int i = 0;
  int isNegative = 0;

  // Extract digits in reverse order
  do {
    str[i++] = num % 10 + '0';
    num /= 10;
  } while (num != 0);

  str[i] = '\0'; // Null-terminate the string

  // Reverse the string
  int start = 0;
  int end = i - 1;
  while (start < end) {
    char temp = str[start];
    str[start] = str[end];
    str[end] = temp;
    start++;
    end--;
  }
  buf_cpy(TEXT_DATA + 64, TEXT_DATA, 64 * 31);
  buf_cpy(TEXT_DATA, str, i);
}

void c_interrupt_handler(uint32_t mcause) {}

extern char _heap_base[];
extern char _stack[];

void *_sbrk(uint32_t numbytes) {
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

typedef enum {
  SYSCALL_GET_MTIME,
  SYSCALL_U32_PRINT,
  SYSCALL_MALLOC,
  SYSCALL_SET_MODE,
  SYSCALL_GET_CONTROLLER,
  SYSCALL_GET_PX_BG_BUF,
  SYSCALL_SET_BG_CONTROLS,
  SYSCALL_GET_BG_PALETTE,
} syscall;

uint32_t c_system_call(uint32_t arg0, uint32_t arg1, uint32_t arg2,
                       uint32_t arg3, uint32_t arg4, uint32_t call) {
  switch (call) {
  case SYSCALL_GET_MTIME:
    return MTIME_LOW;
  case SYSCALL_U32_PRINT:
    u32_print(arg0);
    return 0;
  case SYSCALL_MALLOC:
    return (uint32_t *)_sbrk(arg0);
  case SYSCALL_SET_MODE:
    MODE_CONTROL = arg0;
    return 0;
  case SYSCALL_GET_CONTROLLER:
    return CONTROLLER;
  case SYSCALL_GET_PX_BG_BUF:
    return (uint32_t)(BG_BUFS + (BG_BUF_SIZE * arg0));
  case SYSCALL_SET_BG_CONTROLS:
    BG_CONTROLS[arg0] = arg1;
    return 0;
  case SYSCALL_GET_BG_PALETTE: // get background palette
    return (uint32_t)(BG_PALETTES + (BG_PALETTE_SIZE * arg0));
  }
  return 1;
}
