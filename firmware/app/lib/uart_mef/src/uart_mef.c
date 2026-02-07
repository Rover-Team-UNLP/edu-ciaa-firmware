#include "uart_mef.h"

static struct
{
    uart_state_t state;
    parsed_cmd_t current_cmd;
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

        uint16_t motor1 = 0, motor2 = 0;
        if (fsm_context.current_cmd.cmd.type != COMMAND_STOP) {
            switch (fsm_context.current_cmd.cmd.intensity)
            {
            case INTENSITY_LOW:
                motor1 = 33;
                motor2 = 33;
                break;
            case INTENSITY_MEDIUM:
                motor1 = 66;
                motor2 = 66;
                break;
            case INTENSITY_HIGH:
                motor1 = 100;
                motor2 = 100;
                break;
            default:
                break;
            }
        }

        // NOTA: motor1 = izquierda
        //       motor2 = derecha
        switch (fsm_context.current_cmd.cmd.type)
        {
        case COMMAND_MOVE_FORWARD:
            Motor_SetSpeed(motor1, motor2);
            break;
        case COMMAND_MOVE_BACKWARDS:
            Motor_SetSpeed(-motor1, -motor2);
            break;
        case COMMAND_MOVE_LEFT:
            Motor_SetSpeed(motor1, -motor2);
            break;
        case COMMAND_MOVE_RIGHT:
            Motor_SetSpeed(-motor1, motor2);
            break;
        case COMMAND_STOP:
            Motor_SetSpeed(0, 0);
            break;
        default:
            break;
        }

        Board_LED_Set(LED_1, false);
        uart_request_command();
        fsm_context.state = UART_STATE_IDLE;
        break;

    case UART_STATE_ERROR:
        Board_LED_Set(LED_1, false);
        Motor_emergency_stop();
        uart_request_command();
        fsm_context.state = UART_STATE_IDLE;
        break;

    default:
        fsm_context.state = UART_STATE_INIT;
        break;
    }
}
