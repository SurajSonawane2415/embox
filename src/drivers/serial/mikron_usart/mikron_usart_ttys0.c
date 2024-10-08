/**
 * @file
 * @date 04 aug 2015
 * @author: Anton Bondarev
 */

#include <drivers/common/memory.h>
#include <drivers/serial/uart_dev.h>
#include <drivers/ttys.h>
#include <embox/unit.h>
#include <framework/mod/options.h>
#include <kernel/irq.h>
#include <util/macro.h>

EMBOX_UNIT_INIT(uart_init);

#define UART_BASE OPTION_GET(NUMBER, base_addr)
#define IRQ_NUM   OPTION_GET(NUMBER, irq_num)
#define BAUD_RATE OPTION_GET(NUMBER, baud_rate)
#define TTY_NAME  ttyS0

extern irq_return_t uart_irq_handler(unsigned int irq_nr, void *data);

extern const struct uart_ops mikron_usart_ops;

static struct uart uart0 = {
    .dev_name = MACRO_STRING(TTY_NAME),
    .uart_ops = &mikron_usart_ops,
    .irq_num = IRQ_NUM,
    .base_addr = UART_BASE,
    .params = ((struct uart_params){
        .baud_rate = BAUD_RATE,
        .uart_param_flags = UART_PARAM_FLAGS_8BIT_WORD
                            | UART_PARAM_FLAGS_USE_IRQ,
    }),
};

static const struct uart_params uart_defparams = {
    .baud_rate = BAUD_RATE,
    .uart_param_flags = UART_PARAM_FLAGS_8BIT_WORD | UART_PARAM_FLAGS_USE_IRQ,
};

static int uart_init(void) {
	return uart_register(&uart0, &uart_defparams);
}

PERIPH_MEMORY_DEFINE(mikron_usart, UART_BASE, 0x30);

STATIC_IRQ_ATTACH(IRQ_NUM, uart_irq_handler, &uart0);

TTYS_DEF(TTY_NAME, &uart0);
