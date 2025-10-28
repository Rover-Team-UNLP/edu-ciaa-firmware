#include "uart_comm.h"
#include "chip.h"
#include "board.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// --- Definiciones del Protocolo (Compatible con ESP32) ---
#define FRAME_START_CHAR 'S'
#define FRAME_END_CHAR 'E'
#define FRAME_SEPARATOR ':'

// IDs de comandos (deben coincidir con rover_cmd_type_t del ESP32)
typedef enum
{
    CMD_MOVE_FORWARD = 0,
    CMD_MOVE_BACKWARDS = 1,
    CMD_MOVE_LEFT = 2,
    CMD_MOVE_RIGHT = 3
} cmd_type_t;

// IDs de respuestas (deben coincidir con uart_resp_id_t del ESP32)
typedef enum
{
    RESP_ACK = 0,
    RESP_READY = 1,
    RESP_NACK = 2,
    RESP_ERR_INVALID_COMMAND = 3,
    RESP_ERR_INVALID_PARAMS = 4
} response_type_t;

// --- Variables Internas ---
static volatile uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t rx_write_index = 0;
static volatile bool new_command_received = false;
static RoverCommand last_received_command;

// --- Prototipos Privados ---
static bool parse_command_string(const char *buffer, RoverCommand *command);
static void send_response(response_type_t resp_type, uint16_t cmd_id);

// --- Handler de Interrupción ---
void UART2_IRQHandler(void)
{
    while (Chip_UART_ReadLineStatus(LPC_USART2) & UART_LSR_RDR)
    {
        uint8_t received_byte = Chip_UART_ReadByte(LPC_USART2);

        // Ignorar si el comando anterior no fue procesado
        if (new_command_received)
        {
            continue;
        }

        // Detectar inicio de trama
        if (received_byte == FRAME_START_CHAR)
        {
            rx_write_index = 0;
            memset((void *)rx_buffer, 0, UART_RX_BUFFER_SIZE);
            continue;
        }

        // Detectar fin de trama
        if (received_byte == FRAME_END_CHAR && rx_write_index > 0)
        {
            rx_buffer[rx_write_index] = '\0';
            last_received_command.valid = parse_command_string((const char *)rx_buffer, &last_received_command);
            new_command_received = true;
            rx_write_index = 0;
        }
        // Almacenar byte
        else if (rx_write_index < (UART_RX_BUFFER_SIZE - 1))
        {
            rx_buffer[rx_write_index++] = received_byte;
        }
        // Desborde
        else
        {
            rx_write_index = 0;
            memset((void *)rx_buffer, 0, UART_RX_BUFFER_SIZE);
        }
    }
}

// --- Funciones Públicas ---
void uart_init(uint32_t baudRate)
{
    Chip_SCU_PinMuxSet(7, 1, (SCU_MODE_PULLDOWN | SCU_MODE_FUNC6));
    Chip_SCU_PinMuxSet(7, 2, (SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS | SCU_MODE_FUNC6));

    Chip_UART_Init(LPC_USART2);
    Chip_UART_SetBaudFDR(LPC_USART2, baudRate);
    Chip_UART_ConfigData(LPC_USART2, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS));
    Chip_UART_TXEnable(LPC_USART2);
    Chip_UART_SetupFIFOS(LPC_USART2, (UART_FCR_FIFO_EN | UART_FCR_RX_RS | UART_FCR_TX_RS | UART_FCR_TRG_LEV0));
    Chip_UART_IntEnable(LPC_USART2, UART_IER_RBRINT);

    NVIC_SetPriority(USART2_IRQn, 6);
    NVIC_EnableIRQ(USART2_IRQn);

    rx_write_index = 0;
    new_command_received = false;
    last_received_command.valid = false;
    memset((void *)rx_buffer, 0, UART_RX_BUFFER_SIZE);

    // Enviar RESP_READY al iniciar para indicar que estamos listos
    send_response(RESP_READY, 0);
}

bool uart_is_new_command_available(void)
{
    return new_command_received;
}

void uart_get_received_command(RoverCommand *cmd)
{
    if (cmd == NULL)
        return;

    __disable_irq();
    if (new_command_received)
    {
        memcpy(cmd, (const void *)&last_received_command, sizeof(RoverCommand));
        new_command_received = false;
        last_received_command.valid = false;
    }
    else
    {
        cmd->valid = false;
    }
    __enable_irq();
}

void uart_send_string_blocking(const char *str)
{
    Chip_UART_SendBlocking(LPC_USART2, str, strlen(str));
}

// --- Funciones Privadas ---

/**
 * @brief Parsea comando en formato: <CMD_ID>:<CMD_ID_NUM>:[params]
 * Ejemplo: "0:123:" (MOVE_FORWARD con ID 123)
 */
static bool parse_command_string(const char *buffer, RoverCommand *command)
{
    if (buffer == NULL || command == NULL)
    {
        return false;
    }

    uint8_t cmd_type;
    uint16_t cmd_id;

    // Formato esperado del ESP32: "<CMD_TYPE>:<CMD_ID>:"
    // Ejemplo: "0:123:" para CMD_MOVE_FORWARD con ID 123
    int items = sscanf(buffer, "%hhu:%hu:", &cmd_type, &cmd_id);

    if (items != 2)
    {
        send_response(RESP_ERR_INVALID_COMMAND, 0);
        return false;
    }

    // Validar tipo de comando
    if (cmd_type > CMD_MOVE_RIGHT)
    {
        send_response(RESP_ERR_INVALID_COMMAND, cmd_id);
        return false;
    }

    // Guardar información del comando
    command->cmd_type = cmd_type;
    command->cmd_id = cmd_id;

    // Mapear comandos numéricos a acciones de motor
    // Estos valores son ejemplos, ajústalos según tu hardware
    switch (cmd_type)
    {
    case CMD_MOVE_FORWARD:
        command->speed_M1 = 100;
        command->speed_M2 = 100;
        break;
    case CMD_MOVE_BACKWARDS:
        command->speed_M1 = -100;
        command->speed_M2 = -100;
        break;
    case CMD_MOVE_LEFT:
        command->speed_M1 = -50;
        command->speed_M2 = 50;
        break;
    case CMD_MOVE_RIGHT:
        command->speed_M1 = 50;
        command->speed_M2 = -50;
        break;
    default:
        send_response(RESP_ERR_INVALID_COMMAND, cmd_id);
        return false;
    }

    // Enviar ACK
    send_response(RESP_ACK, cmd_id);
    return true;
}

/**
 * @brief Envía respuesta en formato: S:<RESP_TYPE>:<CMD_ID>:E
 * Ejemplo: "S:0:123:E" (ACK para comando con ID 123)
 */
static void send_response(response_type_t resp_type, uint16_t cmd_id)
{
    char response[20];
    snprintf(response, sizeof(response), "S:%d:%u:E", resp_type, cmd_id);
    uart_send_string_blocking(response);
}