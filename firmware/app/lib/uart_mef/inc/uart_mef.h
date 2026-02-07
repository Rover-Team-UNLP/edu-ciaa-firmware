#include "uart_comm.h"
#include "board.h"
#include <motor.h>

typedef enum
{
    UART_STATE_INIT,
    UART_STATE_IDLE,
    UART_STATE_RECEIVING,
    UART_STATE_CHECK_MESSAGE,
    UART_STATE_PROCESS,
    UART_STATE_ERROR,
} uart_state_t;

void uart_mef_update(void);
void uart_mef_init(void);
