#include "uart_debug.h"
#include "chip.h"
#include <string.h>

#ifdef UART_DEBUG_ENABLE
static void uart_debug_write_char(char ch)
{
    while ((Chip_UART_ReadLineStatus(LPC_USART2) & UART_LSR_THRE) == 0) {}
    Chip_UART_SendByte(LPC_USART2, (uint8_t)ch);
}

void uart_debug_init(uint32_t baud_rate)
{
    Chip_SCU_PinMuxSet(UART_DEBUG_TX_PORT, UART_DEBUG_TX_PIN, UART_DEBUG_TX_MODE);

    Chip_UART_Init(LPC_USART2);
    Chip_UART_SetBaudFDR(LPC_USART2, baud_rate);
    Chip_UART_ConfigData(LPC_USART2, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS));
    Chip_UART_TXEnable(LPC_USART2);
    Chip_UART_SetupFIFOS(LPC_USART2, (UART_FCR_FIFO_EN | UART_FCR_TX_RS));
}

void uart_debug_write(const char *str)
{
    if (str == NULL)
    {
        return;
    }

    while (*str != '\0')
    {
        uart_debug_write_char(*str++);
    }
}

void uart_debug_write_u32(uint32_t value)
{
    char buf[11];
    uint8_t i = 0;

    if (value == 0)
    {
        uart_debug_write_char('0');
        return;
    }

    while (value > 0 && i < sizeof(buf))
    {
        buf[i++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (i > 0)
    {
        uart_debug_write_char(buf[--i]);
    }
}
#endif
