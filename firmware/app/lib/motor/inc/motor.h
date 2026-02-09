
#ifndef MOTOR_H
#define MOTOR_H

#include <chip.h>
#include <board.h>
#include <sct_pwm_18xx_43xx.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>



/**
 * @brief Inicializa los pines GPIO y el PWM para los motores.
 */
void Motor_Init(void);

/**
 * @brief Controla la velocidad y dirección de los motores.
 * * @param speed_left  Velocidad motor izquierdo (-100 a 100).
 * @param speed_right Velocidad motor derecho (-100 a 100).
 *  * Positivo = Adelante, Negativo = Atrás, 0 = Stop.
 */
void Motor_SetSpeed(int16_t speed_left, int16_t speed_right);

/**
 * @brief Detiene ambos motores inmediatamente.
 */
void Motor_Stop(void);


void Motor_emergency_stop(void);

#endif