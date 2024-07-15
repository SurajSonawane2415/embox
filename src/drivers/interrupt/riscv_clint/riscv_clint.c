/**
 * @file
 *
 * @brief Implementation of the RISC-V Core Local Interruptor (CLINT) for interrupt control, including support for SiFive multi-hart architectures.
 * 
 * @date 05.07.2024
 * @authored by Suraj Ravindra Sonawane
 */

#include <stdint.h>
#include <drivers/irqctrl.h>
#include <asm/regs.h>
#include <hal/reg.h>

#define CLINT_ADDR      OPTION_GET(NUMBER, base_addr)
#define MSIP_OFFSET     OPTION_GET(NUMBER, msip_offset)

#define MSIP_ADDR       (CLINT_ADDR + MSIP_OFFSET)

#ifdef SIFIVE_CLINT
#define MAX_HART        5
#endif

/**
 * Initializes the CLINT.
 *
 * This function initializes the CLINT by clearing the MSIP (Software Interrupt).
 *
 * @return 0 on success.
 */
static int clint_init(void) {
#ifdef SIFIVE_CLINT
    // Initial configuration: clear MSIP for all harts
    for (int hart_id = 0; hart_id < MAX_HART; hart_id++) {
        REG32_STORE(MSIP_ADDR + (hart_id * 4), 0);
    }
#else
    // Initial configuration: clear MSIP for a single hart
    REG32_STORE(MSIP_ADDR, 0);
#endif
    enable_software_interrupts();
    return 0;
}

/**
 * Configures the Software Interrupt (MSIP).
 *
 * @param value The value (0 or 1) to set for MSIP.
 * @param hart_id The ID of the hart to configure MSIP for (only used if SIFIVE_CLINT is defined).
 */
void clint_configure_msip(uint8_t value
#ifdef SIFIVE_CLINT
    , int hart_id
#endif
) {
#ifdef SIFIVE_CLINT
    if (hart_id < MAX_HART) {
        REG32_STORE(MSIP_ADDR + (hart_id * 4), value & 1); // Write the LSB of 'value' to the MSIP register of the specified hart
    }
#else
    REG32_STORE(MSIP_ADDR, value & 1); // Write the least significant bit of 'value' to MSIP_ADDR
#endif
}

#ifdef SIFIVE_CLINT
IRQCTRL_DEF(sifive_clint, clint_init);
#else
IRQCTRL_DEF(riscv_clint, clint_init);
#endif