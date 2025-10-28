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

void uart_init(uint32_t baudRate);
bool uart_is_new_command_available(void);
void uart_get_received_command(RoverCommand *cmd);
void uart_send_string_blocking(const char *str);

#endif