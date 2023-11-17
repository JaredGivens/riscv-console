#include <stdint.h>
#include <string.h>

extern uint8_t _erodata[];
extern uint8_t _data[];
extern uint8_t _edata[];
extern uint8_t _sdata[];
extern uint8_t _esdata[];
extern uint8_t _bss[];
extern uint8_t _ebss[];

volatile void (*timer_callback)(void) = NULL;
volatile uint32_t timer_period = 0;
volatile uint64_t timer_last = 0; // time when callback was last called

void (*video_callback)(void) = NULL;

// Adapted from https://stackoverflow.com/questions/58947716/how-to-interact-with-risc-v-csrs-by-using-gcc-c-code
__attribute__((always_inline)) inline uint32_t csr_mstatus_read(void)
{
    uint32_t result;
    asm volatile("csrr %0, mstatus" : "=r"(result));
    return result;
}

__attribute__((always_inline)) inline void csr_mstatus_write(uint32_t val)
{
    asm volatile("csrw mstatus, %0" : : "r"(val));
}

__attribute__((always_inline)) inline void csr_write_mie(uint32_t val)
{
    asm volatile("csrw mie, %0" : : "r"(val));
}

__attribute__((always_inline)) inline void csr_enable_interrupts(void)
{
    asm volatile("csrsi mstatus, 0x8");
}

__attribute__((always_inline)) inline void csr_disable_interrupts(void)
{
    asm volatile("csrci mstatus, 0x8");
}

#define MTIME_LOW (*((volatile uint32_t *)0x40000008))
#define MTIME_HIGH (*((volatile uint32_t *)0x4000000C))
#define MTIMECMP_LOW (*((volatile uint32_t *)0x40000010))
#define MTIMECMP_HIGH (*((volatile uint32_t *)0x40000014))
#define CONTROLLER (*((volatile uint32_t *)0x40000018))

#define INTERRUPT_ENABLE (*((volatile uint32_t *)0x40000000))
#define INTERRUPT_PENDING (*((volatile uint32_t *)0x40000004))

void init(void)
{
    uint8_t *Source = _erodata;
    uint8_t *Base = _data < _sdata ? _data : _sdata;
    uint8_t *End = _edata > _esdata ? _edata : _esdata;

    while (Base < End)
    {
        *Base++ = *Source++;
    }
    Base = _bss;
    End = _ebss;
    while (Base < End)
    {
        *Base++ = 0;
    }

    csr_write_mie(0x888);               // Enable all interrupt soruces
    csr_enable_interrupts();            // Global interrupt enable
    *(uint32_t *)0x40000000 = 0b11111u; // enable chipset interrupts

    MTIMECMP_LOW = 1;
    MTIMECMP_HIGH = 0;
}

extern volatile int global;
extern volatile uint32_t controller_status;

void c_interrupt_handler(uint32_t mcause)
{
    if (mcause == 0x80000007) // machine timer interrupt
    {
        uint64_t NewCompare = (((uint64_t)MTIMECMP_HIGH) << 32) | MTIMECMP_LOW;
        NewCompare += 100;
        MTIMECMP_HIGH = NewCompare >> 32;
        MTIMECMP_LOW = NewCompare;
        global++;
        controller_status = CONTROLLER;
        strcpy((char *)(0x50000000 + 0xF4800), "Interrupt:: Timer");

        if(timer_callback != NULL){
            if(timer_callback != NULL && (NewCompare - timer_last) > timer_period){
                timer_last = NewCompare;
                timer_callback();
            }
        }
    }

    if (mcause == 0x8000000b) // external(chipset) interrupt
    {
        uint32_t pending = INTERRUPT_PENDING;
        if (pending & 0b1)
        { // cartridge interrupt
        }
        if (pending & 0b10)
        { // video interrupt
            if (video_callback != NULL)
                video_callback();
        }
        if (pending & 0b100)
        { // command interrupt
            if(timer_period < 10000){
                timer_period = 10000;
            } else timer_period = 1000;
        }
        INTERRUPT_PENDING = pending; // clear interrupts that have been handled.
    }
}

uint32_t c_system_call(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t call)
{
    if (1 == call)
    {
        return global;
    }
    else if (2 == call)
    {
        return CONTROLLER;
    }
    else if (3 == call)
    { // set timer callback
        timer_callback = (void (*)(void))arg0;
        timer_period = arg1;
        timer_last = (((uint64_t)MTIMECMP_HIGH) << 32) | MTIMECMP_LOW;
    }
    else if (4 == call)
    { // set video callback
        video_callback = (void (*)(void))arg0;
    }

    return -1;
}
