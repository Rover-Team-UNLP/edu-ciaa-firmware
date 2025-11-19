#ifndef UART_COMM_H
#define UART_COMM_H

#include <stdint.h>
#include <stdbool.h>

#define UART_RX_BUFFER_SIZE 64
#define UART_BAUD_RATE 115200

typedef struct
{
    uint8_t cmd_type; // Tipo de comando (0-3)
    uint16_t cmd_id;  // ID del comando para tracking
    int16_t speed_M1; // Velocidad motor 1 (-100 a 100)
    int16_t speed_M2; // Velocidad motor 2 (-100 a 100)
    bool valid;       // Indica si el comando es válido
} RoverCommand;

// --- Funciones Básicas ---

/**
 * @brief Inicializa la uart con el baudRate especificado
 *
 * @param baudRate El baudRate
 */
void uart_init(uint32_t baudRate);

/**
 * @brief Verifica si hay un comando disponible en el buffer
 *
 * @returns True si hay, False si no.
 */
bool uart_is_new_command_available(void);

/**
 * @brief Parsea el comando al tipo RoverCommand.
 *
 * @param cmd Puntero al struct donde almacenar el comando.
 */
void uart_get_received_command(RoverCommand *cmd);

/**
 * @brief Envia una string a UART de forma bloqueante.
 *
 * @param str String a enviar por UART.
 */
void uart_send_string_blocking(const char *str);

// --- Funciones de Alto Nivel ---

/**
 * @brief Solicita un nuevo comando al ESP32 enviando RESP_READY
 */
void uart_request_command(void);

/**
 * @brief Envía un ACK (confirmación) al ESP32 para un comando específico
 * @param cmd_id ID del comando que se está confirmando
 */
void uart_send_ack(uint16_t cmd_id);

/**
 * @brief Envía un NACK (rechazo genérico) al ESP32
 * @param cmd_id ID del comando que se está rechazando
 */
void uart_send_nack(uint16_t cmd_id);

/**
 * @brief Envía error de comando inválido al ESP32
 * @param cmd_id ID del comando que generó el error
 */
void uart_send_error_invalid_command(uint16_t cmd_id);

/**
 * @brief Envía error de parámetros inválidos al ESP32
 * @param cmd_id ID del comando que generó el error
 */
void uart_send_error_invalid_params(uint16_t cmd_id);

#endif