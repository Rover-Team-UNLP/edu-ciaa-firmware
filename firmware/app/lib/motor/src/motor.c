#include "motor.h"
#include "board.h"
#include "chip.h"
#include "sct_pwm_18xx_43xx.h"

// =================================================================
//                    DEFINICIÓN DE PINES 
// =================================================================

// PWM M1 -> GPIO8 (P6_12)
#define M1_PWM_PORT      6
#define M1_PWM_PIN       12
#define M1_PWM_FUNC      SCU_MODE_FUNC1  
#define M1_PWM_INDEX     1               
#define M1_PWM_SCT_OUT   7              

#define M1_IN1_SCU_PORT  6
#define M1_IN1_SCU_PIN   4
#define M1_IN1_FUNC      SCU_MODE_FUNC0
#define M1_IN1_GPIO_P    3
#define M1_IN1_GPIO_B    3

#define M1_IN2_SCU_PORT  6
#define M1_IN2_SCU_PIN   7
#define M1_IN2_FUNC      SCU_MODE_FUNC4
#define M1_IN2_GPIO_P    5
#define M1_IN2_GPIO_B    15

// --- MOTOR 2 (PWM: GPIO2 | DIR: GPIO5, GPIO7) ---

#define M2_PWM_PORT      6
#define M2_PWM_PIN       5
#define M2_PWM_FUNC      SCU_MODE_FUNC1  
#define M2_PWM_INDEX     2
#define M2_PWM_SCT_OUT   6

#define M2_IN1_SCU_PORT  6
#define M2_IN1_SCU_PIN   9
#define M2_IN1_FUNC      SCU_MODE_FUNC0
#define M2_IN1_GPIO_P    3
#define M2_IN1_GPIO_B    5

#define M2_IN2_SCU_PORT  6
#define M2_IN2_SCU_PIN   11
#define M2_IN2_FUNC      SCU_MODE_FUNC0
#define M2_IN2_GPIO_P    3
#define M2_IN2_GPIO_B    7

#define PWM_FREQ_HZ      1000

static void Motor_SetRaw(uint8_t index, int16_t speed, 
                         uint8_t p1, uint8_t b1, 
                         uint8_t p2, uint8_t b2);

void Motor_Init(void)
{
    //desactivo el pin GPIO0
    Chip_SCU_PinMuxSet(0x6, 1, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0)); // P6_1 como GPIO
    Chip_GPIO_SetDir(LPC_GPIO_PORT, 3, (1 << 0), 0);                                     // 0 = Entrada (Input)

    Chip_SCU_PinMuxSet(M1_IN1_SCU_PORT, M1_IN1_SCU_PIN, (SCU_MODE_INACT | M1_IN1_FUNC));
    Chip_SCU_PinMuxSet(M1_IN2_SCU_PORT, M1_IN2_SCU_PIN, (SCU_MODE_INACT | M1_IN2_FUNC));
    Chip_SCU_PinMuxSet(M2_IN1_SCU_PORT, M2_IN1_SCU_PIN, (SCU_MODE_INACT | M2_IN1_FUNC));
    Chip_SCU_PinMuxSet(M2_IN2_SCU_PORT, M2_IN2_SCU_PIN, (SCU_MODE_INACT | M2_IN2_FUNC));

    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, M1_IN1_GPIO_P, M1_IN1_GPIO_B);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, M1_IN2_GPIO_P, M1_IN2_GPIO_B);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, M2_IN1_GPIO_P, M2_IN1_GPIO_B);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, M2_IN2_GPIO_P, M2_IN2_GPIO_B);

    Chip_SCU_PinMuxSet(M1_PWM_PORT, M1_PWM_PIN, (SCU_MODE_INACT | M1_PWM_FUNC));
    Chip_SCU_PinMuxSet(M2_PWM_PORT, M2_PWM_PIN, (SCU_MODE_INACT | M2_PWM_FUNC));

    Chip_SCTPWM_Init(LPC_SCT);
    Chip_SCTPWM_SetRate(LPC_SCT, PWM_FREQ_HZ);

    Chip_SCTPWM_SetOutPin(LPC_SCT, M1_PWM_INDEX, M1_PWM_SCT_OUT);
    Chip_SCTPWM_SetOutPin(LPC_SCT, M2_PWM_INDEX, M2_PWM_SCT_OUT);

    Chip_SCTPWM_SetDutyCycle(LPC_SCT, M1_PWM_INDEX, 0);
    Chip_SCTPWM_SetDutyCycle(LPC_SCT, M2_PWM_INDEX, 0);
    Chip_SCTPWM_Start(LPC_SCT);
}

void Motor_SetSpeed(int16_t speed_m1, int16_t speed_m2)
{
    Motor_SetRaw(M1_PWM_INDEX, speed_m1, M1_IN1_GPIO_P, M1_IN1_GPIO_B, M1_IN2_GPIO_P, M1_IN2_GPIO_B);
    Motor_SetRaw(M2_PWM_INDEX, speed_m2, M2_IN1_GPIO_P, M2_IN1_GPIO_B, M2_IN2_GPIO_P, M2_IN2_GPIO_B);
    UART_DEBUG_LOG_LN("Motor 1: %d", speed_m1);
    UART_DEBUG_LOG_LN("Motor 2: %d", speed_m2);
}

static void Motor_SetRaw(uint8_t index, int16_t speed, 
                         uint8_t p1, uint8_t b1, 
                         uint8_t p2, uint8_t b2)
{
    if (speed > 100) speed = 100;
    if (speed < -100) speed = -100;
    
    uint8_t duty = (speed >= 0) ? speed : -speed;
    
    if (speed > 0) {
        Chip_GPIO_SetPinState(LPC_GPIO_PORT, p1, b1, true);
        Chip_GPIO_SetPinState(LPC_GPIO_PORT, p2, b2, false);
    } else if (speed < 0) {
        Chip_GPIO_SetPinState(LPC_GPIO_PORT, p1, b1, false);
        Chip_GPIO_SetPinState(LPC_GPIO_PORT, p2, b2, true);
    } else {
        Chip_GPIO_SetPinState(LPC_GPIO_PORT, p1, b1, false);
        Chip_GPIO_SetPinState(LPC_GPIO_PORT, p2, b2, false);
    }
    
    
    uint32_t duty_ticks = (Chip_SCTPWM_GetTicksPerCycle(LPC_SCT) * duty) / 100;
    
    Chip_SCTPWM_SetDutyCycle(LPC_SCT, index, duty_ticks);
}   

void Motor_emergency_stop(void)
{
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, M1_IN1_GPIO_P, M1_IN1_GPIO_B, true);
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, M1_IN2_GPIO_P, M1_IN2_GPIO_B, true);
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, M2_IN1_GPIO_P, M2_IN1_GPIO_B, true);
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, M2_IN2_GPIO_P, M2_IN2_GPIO_B, true);

    uint32_t ticks = Chip_SCTPWM_PercentageToTicks(LPC_SCT, (uint8_t)100);
    Chip_SCTPWM_SetDutyCycle(LPC_SCT, M1_PWM_INDEX, ticks);
    Chip_SCTPWM_SetDutyCycle(LPC_SCT, M2_PWM_INDEX, ticks);
}