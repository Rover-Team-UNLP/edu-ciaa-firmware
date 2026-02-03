#include "uart_mef.h"

static struct
{
    uart_state_t state;
    RoverCommand current_cmd;
    uint32_t execution_counter;
} fsm_context = {
    .state = UART_STATE_INIT,
    .current_cmd = {0},
    .execution_counter = 0,
};

void uart_mef_init(void)
{
    fsm_context.state = UART_STATE_INIT;
    fsm_context.execution_counter = 0;
}

void uart_mef_update(void)
{
    switch (fsm_context.state)
    {
    case UART_STATE_INIT:
        uart_init(115200);
        fsm_context.state = UART_STATE_IDLE;
        break;
        
        case UART_STATE_IDLE:
        if (uart_is_new_command_available())
        {
            Board_LED_Set(LED_1, true);
            uart_get_received_command(&fsm_context.current_cmd);
            
            if (fsm_context.current_cmd.valid)
                fsm_context.state = UART_STATE_PROCESS;
            else
                fsm_context.state = UART_STATE_ERROR;
            fsm_context.execution_counter = 0;
        } 
        else if (fsm_context.execution_counter % 1000 == 0)
            uart_request_command(); 
        fsm_context.execution_counter++; // Only used in idle state
        break;

    case UART_STATE_PROCESS:
        // TODO procesar el comando pasandoselo a los motores
        // El ultimo comando recibido esta en fsm_context.current_cmd

        Board_LED_Set(LED_1, false);
        uart_request_command();
        fsm_context.state = UART_STATE_IDLE;
        break;

    case UART_STATE_ERROR:
        Board_LED_Set(LED_1, false);
        // TODO parar motores
        uart_request_command();
        fsm_context.state = UART_STATE_IDLE;
        break;

    default:
        fsm_context.state = UART_STATE_INIT;
        break;
    }
}
