/**
 * @file
 *
 * @brief RISC-V built-in timer with RISC-V and SiFive CLINT integration.
 *
 * @date 12.12.2019
 * @date 14.07.2024 (CLINT timer integration)
 * @authored by Anastasia Nizharadze
 * @authored by Suraj Ravindra Sonawane (CLINT timer integration)
 */

#include <errno.h>
#include <stdint.h>

#include <asm/interrupts.h>
#include <asm/regs.h>
#include <framework/mod/options.h>
#include <hal/clock.h>
#include <hal/reg.h>
#include <hal/system.h>
#include <kernel/irq.h>
#include <kernel/time/clock_source.h>
#include <kernel/time/time.h>

#define COUNT_OFFSET  (RTC_CLOCK / JIFFIES_PERIOD)
#define RTC_CLOCK     OPTION_GET(NUMBER, rtc_freq)

#define MTIME         OPTION_GET(NUMBER, base_mtime)
#define MTIMECMP      OPTION_GET(NUMBER, base_mtimecmp)

#ifdef SIFIVE_CLINT
#define MTIMECMP_HART(hart)  (MTIMECMP + (hart) * 8)
#define MAX_HART        5
#else
#define MTIMECMP_HART(hart)  MTIMECMP
#endif

#define OPENSBI_TIMER 0x54494D45

/**
 * Initializes the CLINT timer module.
 *
 * This function initializes the CLINT timer by setting MTIMECMP to its maximum value and initializing MTIME to 0.
 *
 * @return 0 on success.
 */
static int clint_timer_init(void) {
#ifdef SIFIVE_CLINT
    int hart;
    for (hart = 0; hart < MAX_HART; hart++) {
        REG64_STORE(MTIMECMP_HART(hart), 0xFFFFFFFFFFFFFFFF); // Set MTIMECMP to max value for each hart
    }
#else
    REG64_STORE(MTIMECMP_HART(0), 0xFFFFFFFFFFFFFFFF); // Set MTIMECMP to max value
#endif
    REG64_STORE(MTIME, 0); // Initialize MTIME to 0
    return 0;
}

/**
 * Sets the MTIMECMP register value for the specific hart.
 *
 * This function sets the MTIMECMP register to the provided 64-bit value for the specified hart.
 *
 * @param hart The hart id.
 * @param value The value to set for MTIMECMP.
 */
static void clint_set_mtimecmp(int hart, uint64_t value) {
    REG64_STORE(MTIMECMP_HART(hart), value); // Write 'value' to MTIMECMP for the specified hart
}

/**
 * Retrieves the current value of MTIME.
 *
 * This function reads and returns the current value of MTIME.
 *
 * @return The current value of MTIME.
 */
static uint64_t clint_get_mtime(void) {
    return REG64_LOAD(MTIME); // Read and return the value at MTIME
}

static int clock_handler(unsigned int irq_nr, void *dev_id
#ifdef SIFIVE_CLINT
    , int hart
#endif
) {
#if SMODE
    register uintptr_t a7 asm("a7") = (uintptr_t)(OPENSBI_TIMER);
    register uintptr_t a6 asm("a6") = (uintptr_t)(0);
    register uintptr_t a0 asm("a0") = 0;
    asm volatile("rdtime a0");
    a0 = a0 + COUNT_OFFSET;
    (void)a7;
    (void)a6;
    asm volatile("ecall");
#else
#ifdef SIFIVE_CLINT
    if (hart < MAX_HART) {
        clint_set_mtimecmp(hart, clint_get_mtime() + COUNT_OFFSET);
    }
#else
    clint_set_mtimecmp(0, clint_get_mtime() + COUNT_OFFSET);
#endif
#endif

    clock_tick_handler(dev_id);

    return IRQ_HANDLED;
}

static int riscv_clock_setup(struct clock_source *cs
#ifdef SIFIVE_CLINT
    , int hart
#endif
) {
#if SMODE
    register uintptr_t a7 asm("a7") = (uintptr_t)(OPENSBI_TIMER);
    register uintptr_t a6 asm("a6") = (uintptr_t)(0);
    register uintptr_t a0 asm("a0") = 0;
    asm volatile("rdtime a0");
    a0 = a0 + COUNT_OFFSET;
    (void)a7;
    (void)a6;
    asm volatile("ecall");
#else
#ifdef SIFIVE_CLINT
    if (hart < MAX_HART) {
        clint_set_mtimecmp(hart, clint_get_mtime() + COUNT_OFFSET);
    }
#else
    clint_set_mtimecmp(0, clint_get_mtime() + COUNT_OFFSET);
#endif
#endif

    enable_timer_interrupts();

    return ENOERR;
}

static struct time_event_device riscv_event_device = {
    .set_periodic = riscv_clock_setup,
    .name = "riscv_clk",
    .irq_nr = IRQ_TIMER,
};

CLOCK_SOURCE_DEF(riscv_clk, NULL, NULL, &riscv_event_device, NULL);

RISCV_TIMER_IRQ_DEF(clock_handler, &CLOCK_SOURCE_NAME(riscv_clk));