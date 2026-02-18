#include "uart_mef.h"
#include "uart_debug.h"
#include <stdlib.h>

#define UART_MEF_REQUEST_COMMAND_TICKS 50
#define UART_MEF_TIMEOUT_FB_TICKS 48
#define UART_MEF_TIMEOUT_TURN_TICKS 25
#define UART_MEF_RAMP_MAX_DELTA 15
#define UART_MEF_RAMP_MAX_DELTA_TURN 100

static uint32_t get_motion_timeout_ticks(uint8_t cmd_type)
{
    switch (cmd_type)
    {
    case COMMAND_MOVE_FORWARD:
    case COMMAND_MOVE_BACKWARDS:
        return UART_MEF_TIMEOUT_FB_TICKS;
    case COMMAND_MOVE_LEFT:
    case COMMAND_MOVE_RIGHT:
        return UART_MEF_TIMEOUT_TURN_TICKS;
    default:
        return 0U;
    }
}

static int16_t ramp_velocity_with_delta(int16_t current, int16_t target, uint16_t max_delta)
{
    if (current == target)
        return current;
    
    int16_t delta = target - current;
    int16_t abs_delta = (delta < 0) ? -delta : delta;
    int16_t sign = (delta > 0) ? 1 : -1;
    
    if (abs_delta <= max_delta)
        return target;
    else
        return current + (sign * max_delta);
}

static void led_yellow()
{
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, RED_LED, 0);
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, GREEN_LED, 1);
}

static void led_red()
{
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, RED_LED, 1);
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, GREEN_LED, 0);
}

static struct
{
    uart_state_t state;
    parsed_cmd_t current_cmd;
    uint32_t request_counter;
    uint32_t motion_inactivity_counter;
    uint8_t motion_timeout_active;
    uint8_t active_motion_type;
    int16_t m1_target, m2_target;
    int16_t m1_current, m2_current;
    uint32_t idle_prolonged_counter;
} fsm_context = {
    .state = UART_STATE_INIT,
    .current_cmd = {0},
    .request_counter = 0,
    .motion_inactivity_counter = 0,
    .motion_timeout_active = 0,
    .active_motion_type = COMMAND_STOP,
    .m1_target = 0,
    .m2_target = 0,
    .m1_current = 0,
    .m2_current = 0,
    .idle_prolonged_counter = 0,
};

void uart_mef_init(void)
{
    fsm_context.state = UART_STATE_INIT;
    fsm_context.request_counter = 0;
    fsm_context.motion_inactivity_counter = 0;
    fsm_context.motion_timeout_active = 0;
    fsm_context.active_motion_type = COMMAND_STOP;
    fsm_context.m1_target = 0;
    fsm_context.m2_target = 0;
    fsm_context.m1_current = 0;
    fsm_context.m2_current = 0;
    fsm_context.idle_prolonged_counter = 0;
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
            fsm_context.request_counter = 0;
            fsm_context.motion_inactivity_counter = 0;
        } 
        else
        {
            uint16_t delta_max = (fsm_context.active_motion_type == COMMAND_MOVE_LEFT ||
                                  fsm_context.active_motion_type == COMMAND_MOVE_RIGHT)
                                     ? UART_MEF_RAMP_MAX_DELTA_TURN
                                     : UART_MEF_RAMP_MAX_DELTA;

            fsm_context.m1_current = ramp_velocity_with_delta(fsm_context.m1_current, fsm_context.m1_target, delta_max);
            fsm_context.m2_current = ramp_velocity_with_delta(fsm_context.m2_current, fsm_context.m2_target, delta_max);
            Motor_SetSpeed(fsm_context.m1_current, fsm_context.m2_current);
            
            // LED indicadores de estado
            if (fsm_context.m1_target == 0 && fsm_context.m2_target == 0)
            {
                // Quieto
                if (fsm_context.idle_prolonged_counter < 100)
                {
                    // Normal idle: RED ON, GREEN (amarillo) OFF
                    led_red();
                }
                else
                {
                    // Timeout idle >2s: alterna rojo-amarillo cada 200ms (10 ticks)
                    if ((fsm_context.idle_prolonged_counter / 10) % 2 == 0)
                    {
                        led_red();
                    }
                    else
                    {
                        led_yellow();
                    }
                }
            }
            else
            {
                // En movimiento: RED OFF, GREEN (amarillo) ON
                led_yellow();
                fsm_context.idle_prolonged_counter = 0;
            }
            
            fsm_context.idle_prolonged_counter++;
            
            if (fsm_context.motion_timeout_active)
            {
                uint32_t timeout_ticks = get_motion_timeout_ticks(fsm_context.active_motion_type);

                if (timeout_ticks > 0U)
                {
                    fsm_context.motion_inactivity_counter++;

                    if (fsm_context.motion_inactivity_counter >= timeout_ticks)
                    {
                        fsm_context.m1_target = 0;
                        fsm_context.m2_target = 0;
                        fsm_context.motion_timeout_active = 0;
                        fsm_context.motion_inactivity_counter = 0;
#ifdef UART_DEBUG_ENABLE
                        UART_DEBUG_LOG_LN("MOTION TIMEOUT -> STOP");
#endif
                    }
                }
            }

            fsm_context.request_counter++;
            if (fsm_context.request_counter >= UART_MEF_REQUEST_COMMAND_TICKS)
            {
                uart_request_command();
                fsm_context.request_counter = 0;
#ifdef UART_DEBUG_ENABLE
                UART_DEBUG_LOG_LN("REQUEST COMMAND SENT");
#endif
            }
        }
        break;

    case UART_STATE_PROCESS:
     {   
        int motor1 = 0, motor2 = 0;
        if (fsm_context.current_cmd.cmd.type != COMMAND_STOP) {
            switch (fsm_context.current_cmd.cmd.intensity)
            {
            case INTENSITY_LOW:
                motor1 = 50;
                motor2 = 50;
                UART_DEBUG_LOG_LN("Motor 1 & 2: 33");
                break;
            case INTENSITY_MEDIUM:
                motor1 = 70;
                motor2 = 70;
                UART_DEBUG_LOG_LN("Motor 1 & 2: 66");
                break;
            case INTENSITY_HIGH:
                motor1 = 100;
                motor2 = 100;
                UART_DEBUG_LOG_LN("Motor 1 & 2: 100");
                break;
            default:
                break;
            }
            led_yellow();
        }
        else {
            led_red();
        }

        // NOTA: motor1 = izquierda
        //       motor2 = derecha
        switch (fsm_context.current_cmd.cmd.type)
        {
        case COMMAND_MOVE_FORWARD:
            UART_DEBUG_LOG_LN("COMMAND RECEIVED: FORWARD\n");
            fsm_context.m1_target = -motor1;
            fsm_context.m2_target = -motor2;
            fsm_context.motion_timeout_active = 1;
            fsm_context.active_motion_type = COMMAND_MOVE_FORWARD;
            fsm_context.motion_inactivity_counter = 0;
            break;
        case COMMAND_MOVE_BACKWARDS:
            UART_DEBUG_LOG_LN("COMMAND RECEIVED: BACKWARDS\n");
            fsm_context.m1_target = motor1;
            fsm_context.m2_target = motor2;
            fsm_context.motion_timeout_active = 1;
            fsm_context.active_motion_type = COMMAND_MOVE_BACKWARDS;
            fsm_context.motion_inactivity_counter = 0;
            break;
        case COMMAND_MOVE_LEFT:
            UART_DEBUG_LOG_LN("COMMAND RECEIVED: LEFT\n");
            fsm_context.m1_target = -100;
            fsm_context.m2_target = 100;
            fsm_context.motion_timeout_active = 1;
            fsm_context.active_motion_type = COMMAND_MOVE_LEFT;
            fsm_context.motion_inactivity_counter = 0;
            break;
        case COMMAND_MOVE_RIGHT:
            UART_DEBUG_LOG_LN("COMMAND RECEIVED: RIGHT\n");
            fsm_context.m1_target = 100;
            fsm_context.m2_target = -100;
            fsm_context.motion_timeout_active = 1;
            fsm_context.active_motion_type = COMMAND_MOVE_RIGHT;
            fsm_context.motion_inactivity_counter = 0;
            break;
        case COMMAND_STOP:
            UART_DEBUG_LOG_LN("COMMAND RECEIVED: STOP\n");
            fsm_context.m1_target = 0;
            fsm_context.m2_target = 0;
            fsm_context.motion_timeout_active = 0;
            fsm_context.active_motion_type = COMMAND_STOP;
            fsm_context.motion_inactivity_counter = 0;
            break;
        default:
            fsm_context.m1_target = 0;
            fsm_context.m2_target = 0;
            fsm_context.motion_timeout_active = 0;
            fsm_context.active_motion_type = COMMAND_STOP;
            fsm_context.motion_inactivity_counter = 0;
            break;
        }

        Board_LED_Set(LED_1, false);
        fsm_context.request_counter = 0;
        fsm_context.idle_prolonged_counter = 0;
        fsm_context.state = UART_STATE_IDLE;
    #ifdef UART_DEBUG_ENABLE
        UART_DEBUG_LOG_LN("FSM CMD PROCESSED");
    #endif
        break;
    }

    case UART_STATE_ERROR:
        Board_LED_Set(LED_1, false);
        Motor_emergency_stop();
        fsm_context.request_counter = 0;
        fsm_context.motion_timeout_active = 0;
        fsm_context.active_motion_type = COMMAND_STOP;
        fsm_context.motion_inactivity_counter = 0;
        fsm_context.m1_target = 0;
        fsm_context.m2_target = 0;
        fsm_context.m1_current = 0;
        fsm_context.m2_current = 0;
        fsm_context.idle_prolonged_counter = 0;
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
