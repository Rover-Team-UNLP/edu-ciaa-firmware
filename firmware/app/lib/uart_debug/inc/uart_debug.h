#ifndef UART_DEBUG_H
#define UART_DEBUG_H

#include "chip.h"
#include <stdint.h>

#define UART_DEBUG_ENABLE

#ifndef UART_DEBUG_TX_PORT
#define UART_DEBUG_TX_PORT 7
#endif

#ifndef UART_DEBUG_TX_PIN
#define UART_DEBUG_TX_PIN 0
#endif

#ifndef UART_DEBUG_TX_MODE
#define UART_DEBUG_TX_MODE (SCU_MODE_INACT | SCU_MODE_FUNC6)
#endif

#ifndef UART_DEBUG_BAUD_RATE
#define UART_DEBUG_BAUD_RATE 115200
#endif

#ifdef UART_DEBUG_ENABLE
void uart_debug_init(uint32_t baud_rate);
void uart_debug_write(const char *str);
void uart_debug_write_u32(uint32_t value);

#define UART_DEBUG_LOG(msg) uart_debug_write(msg)
#define UART_DEBUG_LOG_U32(val) uart_debug_write_u32((uint32_t)(val))
#define UART_DEBUG_LOG_LN(msg)         \
    do                                 \
    {                                  \
        uart_debug_write(msg);         \
        uart_debug_write("\r\n");      \
    } while (0)
#else
static inline void uart_debug_init(uint32_t baud_rate)
{
    (void)baud_rate;
}
static inline void uart_debug_write(const char *str)
{
    (void)str;
}
static inline void uart_debug_write_u32(uint32_t value)
{
    (void)value;
}

#define UART_DEBUG_LOG(msg) do { (void)(msg); } while (0)
#define UART_DEBUG_LOG_U32(val) do { (void)(val); } while (0)
#define UART_DEBUG_LOG_LN(msg) do { (void)(msg); } while (0)
#endif

#endif
