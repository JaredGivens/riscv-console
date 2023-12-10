#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
#define LINE_LEN 64

volatile int ticks;

uint8_t *BG_DATAS = (void *)0x50000000;
uint32_t BG_DATA_SIZE = 0x24000;
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
uint32_t timer_period = 100;
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

void *memset(void *dest, uint32_t n, uint32_t size) {
  for (int32_t i = size - 1; i > -1; --i) {
    ((uint8_t *)dest)[i] = n;
  }
  return dest;
}

void *memcpy(void *dest, void *src, uint32_t size) {
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

void printf(const char *fmt, ...) {
  if (MODE_CONTROL & 0b1) {
    return;
  }
  va_list args;
  va_start(args, fmt);
  char line[LINE_LEN] = {0};
  int line_i = 0;
  for (int i = 0; fmt[i] != '\0'; ++i) {
    if (fmt[i] != '%') {
      line[line_i++] = fmt[i];
      continue;
    }
    ++i;
    if (fmt[i] == '\0') {
      break;
    }

    switch (fmt[i]) {
    case 'd': {
      int num = va_arg(args, int);
      int start = line_i;
      // Extract digits in reverse order
      do {
        line[line_i++] = num % 10 + '0';
        num /= 10;
      } while (num != 0);

      // Reverse the string
      int end = line_i - 1;
      while (start < end) {
        char temp = line[start];
        line[start] = line[end];
        line[end] = temp;
        start++;
        end--;
      }
    }
    case 'f':
      break;
    }
  }

  memcpy(TEXT_DATA + LINE_LEN, TEXT_DATA, LINE_LEN * 31);
  memcpy(TEXT_DATA, line, LINE_LEN);
  va_end(args);
}

void init(void) {
  // set bss to zero
  memset(_bss, 0, _ebss - _bss);
  // zero controls
  // might not be neccessary
  memcpy((void *)BG_CONTROLS, 0, 64);
  // copy data rom to data ram
  memcpy(_data, _data_source, _edata - _data);

  csr_write_mie(0x808);    // Enable all timer interrupts
  csr_enable_interrupts(); // Global interrupt enable
  INTERRUPT_ENABLE = 0b10;
  MTIMECMP_LOW = 1;
  MTIMECMP_HIGH = 0;
}

bool running_cartridge = false;
char *error_msg[256] = {"instruction address misaligned\n",
                        "instruction access fault\n", "illegal instruction\n",
                        "load address misalighned\n"
                        "load access fault\n"
                        "store address misaligned\n"
                        "store access fault\n"};
void c_interrupt_handler(uint32_t mcause) {
  if (mcause < 8) {
    MODE_CONTROL = 0;
    printf(error_msg[mcause]);
    return;
  }
  switch (mcause) {
  case 0x80000007: // machine timer interrupt
  {
    uint64_t next_time_cmp = (((uint64_t)MTIMECMP_HIGH) << 32) | MTIMECMP_LOW;
    next_time_cmp += 100;
    MTIMECMP_HIGH = next_time_cmp >> 32;
    MTIMECMP_LOW = next_time_cmp;
    // printf("Timer %d", next_time_cmp);

    if (timer_callback != NULL && (next_time_cmp - timer_last) > timer_period) {
      timer_last = next_time_cmp;
      timer_callback();
    }
    break;
  }
  case 0x8000000b: // external(chipset) interrupt
  {
    uint32_t pending = INTERRUPT_PENDING;
    if (pending & 0b1) {
    }
    if (pending & 0b10) { // video interrupt
      if (video_callback != NULL) {
        video_callback(video_arg);
      }
      // MODE_CONTROL = 0;
      printf("video");
    }
    if (pending & 0b100) { // command interrupt
    }
    INTERRUPT_PENDING = pending; // clear interrupts that have been handled.
    break;
  }
  }
}

typedef enum {
  SYSCALL_GET_MTIME,
  SYSCALL_PRINTF,
  SYSCALL_MEMCPY,
  SYSCALL_SET_MODE,
  SYSCALL_GET_CONTROLLER,
  SYSCALL_GET_PIXEL_BG_DATA,
  SYSCALL_SET_PIXEL_BG_CONTROLS,
  SYSCALL_GET_BG_PALETTE,
  SYSCALL_SET_VIDEO_CALLBACK,
  SYSCALL_SET_TIMER_CALLBACK,
} syscall;

uint32_t c_system_call(uint32_t arg0, uint32_t arg1, uint32_t arg2,
                       uint32_t arg3, uint32_t arg4, uint32_t call) {
  switch (call) {
  case SYSCALL_GET_MTIME:
    return MTIME_LOW;
  case SYSCALL_PRINTF:
    printf((const char *)arg0, arg1, arg2, arg3, arg4);
    return 0;
  case SYSCALL_MEMCPY:
    return (uint32_t)memcpy((char *)arg0, (char *)arg1, arg2);
  case SYSCALL_SET_MODE:
    MODE_CONTROL = arg0;
    return 0;
  case SYSCALL_GET_CONTROLLER:
    return CONTROLLER;
  case SYSCALL_GET_PIXEL_BG_DATA:
    return (uint32_t)(BG_DATAS + (BG_DATA_SIZE * arg0));
  case SYSCALL_SET_PIXEL_BG_CONTROLS:
    BG_CONTROLS[arg0] = arg1;
    return 0;
  case SYSCALL_GET_BG_PALETTE:
    return (uint32_t)(BG_PALETTES + (BG_PALETTE_SIZE * arg0));
  case SYSCALL_SET_VIDEO_CALLBACK:
    video_callback = (void (*)(void *))arg0;
    video_arg = (void *)arg1;
    return 0;
  case SYSCALL_SET_TIMER_CALLBACK:
    timer_callback = (void (*)())arg0;
    timer_period = arg1;
    timer_last = (((uint64_t)MTIMECMP_HIGH) << 32) | MTIMECMP_LOW;
    return 0;
  }
  return 1;
}
