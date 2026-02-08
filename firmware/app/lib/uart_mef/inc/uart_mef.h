#include "uart_comm.h"
#include "board.h"
#include "motor.h"

#define RED_LED 5, 16

#define GREEN_LED 3, 6

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

