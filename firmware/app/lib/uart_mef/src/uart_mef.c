#include "uart_mef.h"
#include "uart_debug.h"

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
#ifdef UART_DEBUG_ENABLE
        UART_DEBUG_LOG_LN("FSM INIT");
#endif
        
        Chip_SCU_PinMuxSet(6, 8, SCU_MODE_INACT | SCU_MODE_FUNC4);
        Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 5, 16);
        Chip_SCU_PinMuxSet(6, 10, SCU_MODE_INACT | SCU_MODE_FUNC0);
        Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 3, 6);
        break;
        
    case UART_STATE_IDLE:
        if (uart_is_new_command_available())
        {
            Board_LED_Set(LED_1, true);
            uart_get_received_command(&fsm_context.current_cmd);
            
            if (fsm_context.current_cmd.valid)
            {
                fsm_context.state = UART_STATE_PROCESS;
 #ifdef UART_DEBUG_ENABLE
                UART_DEBUG_LOG_LN("FSM CMD RECEIVED");
 #endif
            }
            else
            {
                fsm_context.state = UART_STATE_ERROR;
 #ifdef UART_DEBUG_ENABLE
                UART_DEBUG_LOG_LN("FSM CMD INVALID");
 #endif
            }
            fsm_context.execution_counter = 0;
        } 
        else if (fsm_context.execution_counter % 2000 == 0)
        {
            uart_request_command(); 
#ifdef UART_DEBUG_ENABLE
            UART_DEBUG_LOG_LN("REQUEST COMMAND SENT");
#endif
        }
        fsm_context.execution_counter++; // Only used in idle state

      
        break;

    case UART_STATE_PROCESS:
     {   
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
            Chip_GPIO_SetPinState(LPC_GPIO_PORT, RED_LED, 0);
            Chip_GPIO_SetPinState(LPC_GPIO_PORT, GREEN_LED, 1);
        }
        else {
            Chip_GPIO_SetPinState(LPC_GPIO_PORT, RED_LED, 1);
            Chip_GPIO_SetPinState(LPC_GPIO_PORT, GREEN_LED, 0);
        }

        // NOTA: motor1 = izquierda
        //       motor2 = derecha
        switch (fsm_context.current_cmd.cmd.type)
        {
        case COMMAND_MOVE_FORWARD:
            UART_DEBUG_LOG_LN("COMMAND RECEIVED: FORWARD\n");
            Motor_SetSpeed(motor1, motor2);
            break;
        case COMMAND_MOVE_BACKWARDS:
            UART_DEBUG_LOG_LN("COMMAND RECEIVED: BACKWARDS\n");
            Motor_SetSpeed(-motor1, -motor2);
            break;
        case COMMAND_MOVE_LEFT:
            UART_DEBUG_LOG_LN("COMMAND RECEIVED: LEFT\n");
            Motor_SetSpeed(motor1, -motor2);
            break;
        case COMMAND_MOVE_RIGHT:
            UART_DEBUG_LOG_LN("COMMAND RECEIVED: RIGHT\n");
            Motor_SetSpeed(-motor1, motor2);
            break;
        case COMMAND_STOP:
            UART_DEBUG_LOG_LN("COMMAND RECEIVED: STOP\n");
            Motor_SetSpeed(0, 0);
            break;
        default:
            break;
        }

        Board_LED_Set(LED_1, false);
        
        uart_request_command();
        fsm_context.state = UART_STATE_IDLE;
    #ifdef UART_DEBUG_ENABLE
        UART_DEBUG_LOG_LN("FSM CMD PROCESSED");
    #endif
        break;
    }

    case UART_STATE_ERROR:
        Board_LED_Set(LED_1, false);
        Motor_emergency_stop();
        uart_request_command();
        fsm_context.state = UART_STATE_IDLE;
#ifdef UART_DEBUG_ENABLE
        UART_DEBUG_LOG_LN("FSM ERROR");
#endif
        break;

    default:
        fsm_context.state = UART_STATE_INIT;
        break;
    }
}
