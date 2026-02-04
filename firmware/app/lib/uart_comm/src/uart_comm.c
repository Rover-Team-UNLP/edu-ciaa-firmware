#include "uart_comm.h"
#include "chip.h"
#include "board.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <cmsis_43xx.h>

// --- Debug ---
#define DEBUG // Descomentar para habilitar logs de debug

// --- Definiciones del Protocolo (Compatible con ESP32) ---
#define FRAME_START_CHAR 'S'
#define FRAME_END_CHAR 'E'
#define FRAME_SEPARATOR ':'

// --- Variables Internas ---
static volatile uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t rx_write_index = 0;
static volatile bool new_command_received = false;
static parsed_cmd_t last_received_command;

// --- Prototipos Privados ---
static bool parse_command_string(const char *buffer, parsed_cmd_t *command);
static void send_response(uart_resp_id_t resp_type, uint16_t cmd_id);

#ifdef DEBUG
static const char *get_command_name(uint8_t cmd_type);
static const char *get_response_name(uart_resp_id_t resp_type);
static const char* get_command_intensity(uint8_t cmd_type);
#endif

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
            rx_buffer[rx_write_index++] = received_byte;
            continue;
        }

        if (rx_write_index == 0)
        {
            continue;
        }

        if (rx_write_index > 0 && rx_write_index < UART_RX_BUFFER_SIZE - 1)
        {
            rx_buffer[rx_write_index++] = received_byte;

            if (received_byte == FRAME_END_CHAR)
            {
                rx_buffer[rx_write_index] = '\0';

                if (parse_command_string((const char *)rx_buffer, &last_received_command))
                {
                    last_received_command.valid = true;
                    new_command_received = true;
                    Board_LED_Set(LED_1, true);
                }

                rx_write_index = 0;
            }
        }
        else if (rx_write_index >= UART_RX_BUFFER_SIZE - 1)
        {
            send_response(RESP_ERR_INVALID_COMMAND, 0);
            rx_write_index = 0;
#ifdef DEBUG
            uart_send_string_blocking("[ERROR] Buffer RX lleno - trama descartada\n");
#endif
        }
    }
}

// --- Funciones Públicas Básicas ---
void uart_init(uint32_t baudRate)
{
    Board_LED_Set(LED_1, true);
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
    Board_LED_Set(LED_1, false);
}

bool uart_is_new_command_available(void)
{
    return new_command_received;
}

void uart_get_received_command(parsed_cmd_t *cmd)
{
    if (cmd == NULL)
        return;

    __disable_irq();
    if (new_command_received)
    {
        memcpy(cmd, (const void *)&last_received_command, sizeof(parsed_cmd_t));
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

// --- Funciones Públicas de Alto Nivel ---

/**
 * @brief Solicita un nuevo comando al ESP32 enviando RESP_READY
 */
void uart_request_command(void)
{
    send_response(RESP_READY, 0);
}

/**
 * @brief Envía un ACK (confirmación) al ESP32 para un comando específico
 * @param cmd_id ID del comando que se está confirmando
 */
void uart_send_ack(uint16_t cmd_id)
{
    send_response(RESP_ACK, cmd_id);
}

/**
 * @brief Envía un NACK (rechazo genérico) al ESP32
 * @param cmd_id ID del comando que se está rechazando
 */
void uart_send_nack(uint16_t cmd_id)
{
    send_response(RESP_NACK, cmd_id);
}

/**
 * @brief Envía error de comando inválido al ESP32
 * @param cmd_id ID del comando que generó el error
 */
void uart_send_error_invalid_command(uint16_t cmd_id)
{
    send_response(RESP_ERR_INVALID_COMMAND, cmd_id);
}

/**
 * @brief Envía error de parámetros inválidos al ESP32
 * @param cmd_id ID del comando que generó el error
 */
void uart_send_error_invalid_params(uint16_t cmd_id)
{
    send_response(RESP_ERR_INVALID_PARAMS, cmd_id);
}

// --- Funciones Privadas ---

/**
 * @brief Parsea comando en formato: S:<COMMAND_ID>:<COMMAND_INTENSITY>:<COMMAND_ID_NUM>:E
 * Ejemplo: "0:1:123:" (MOVE_FORWARD INTENSITY_MEDIUM con ID 123)
 */
static bool parse_command_string(const char *buffer, parsed_cmd_t *command)
{
    if (buffer == NULL || command == NULL)
    {
        return false;
    }

#ifdef DEBUG
    unsigned int cmd_id;
    unsigned int cmd_type;
    unsigned int cmd_intensity;
#else
    uint16_t cmd_id;
    uint8_t cmd_type;
    uint8_t cmd_intensity;
#endif
    char start_char, end_char;

#ifdef DEBUG
    char debug_buf[80];
    snprintf(debug_buf, sizeof(debug_buf), "[DEBUG] Buffer (len=%u): '", strlen(buffer));
    uart_send_string_blocking(debug_buf);

    // Imprimir cada byte en hexadecimal para debug
    for (size_t i = 0; i < strlen(buffer) && i < 20; i++)
    {
        snprintf(debug_buf, sizeof(debug_buf), "%c", buffer[i]);
        uart_send_string_blocking(debug_buf);
    }
    uart_send_string_blocking("'\n");

    // Mostrar en hex
    uart_send_string_blocking("[DEBUG] Hex: ");
    for (size_t i = 0; i < strlen(buffer) && i < 20; i++)
    {
        snprintf(debug_buf, sizeof(debug_buf), "%02X ", (uint8_t)buffer[i]);
        uart_send_string_blocking(debug_buf);
    }
    uart_send_string_blocking("\n");
#endif

    size_t len = strlen(buffer);
    if (len < 7 || len > 20)
    {
#ifdef DEBUG
        snprintf(debug_buf, sizeof(debug_buf),
                 "[DEBUG] FAIL: Invalid length %u (expected 7-20)\n", len);
        uart_send_string_blocking(debug_buf);
#endif
        send_response(RESP_ERR_INVALID_COMMAND, 0);
        return false;
    }

    // VALIDACIÓN ADICIONAL: Verificar que el formato sea correcto ANTES de sscanf
    // Formato: "S:X:Y:E" donde X es 1 dígito y Y es 1-5 dígitos
    if (buffer[0] != 'S' || buffer[len - 1] != 'E')
    {
#ifdef DEBUG
        snprintf(debug_buf, sizeof(debug_buf),
                 "[DEBUG] FAIL: Invalid start/end chars: '%c'/'%c'\n",
                 buffer[0], buffer[len - 1]);
        uart_send_string_blocking(debug_buf);
#endif
        send_response(RESP_ERR_INVALID_COMMAND, 0);
        return false;
    }

    // Contar separadores ':'
    int separator_count = 0;
    for (size_t i = 0; i < len; i++)
    {
        if (buffer[i] == ':')
        {
            separator_count++;
        }
    }

    if (separator_count != 4)
    {
#ifdef DEBUG
        snprintf(debug_buf, sizeof(debug_buf),
                 "[DEBUG] FAIL: Expected 4 separators, got %d\n", separator_count);
        uart_send_string_blocking(debug_buf);
#endif
        send_response(RESP_ERR_INVALID_COMMAND, 0);
        return false;
    }

#ifdef DEBUG
    int items = sscanf(buffer, "%c:%u:%u:%u:%c", &start_char, &cmd_type, &cmd_intensity, &cmd_id, &end_char);
#else
    int items = sscanf(buffer, "%c:%hhu:%hhu:%hu:%c", &start_char, &cmd_type, &cmd_intensity, &cmd_id, &end_char);
#endif

#ifdef DEBUG
    snprintf(debug_buf, sizeof(debug_buf),
             "[DEBUG] Parsed: items=%d start='%c'(0x%02X) type=%u intensity=%u id=%u end='%c'(0x%02X)\n",
             items, start_char, (uint8_t)start_char, cmd_type, cmd_intensity, cmd_id,
             end_char, (uint8_t)end_char);
    uart_send_string_blocking(debug_buf);
#endif

    if (items != 5 || start_char != 'S' || end_char != 'E')
    {
#ifdef DEBUG
        snprintf(debug_buf, sizeof(debug_buf),
                 "[DEBUG] FAIL: sscanf validation failed\n");
        uart_send_string_blocking(debug_buf);
#endif
        send_response(RESP_ERR_INVALID_COMMAND, 0);
        return false;
    }

    // Validar tipo de comando
    if (cmd_type > COMMAND_STOP)
    {
#ifdef DEBUG
        snprintf(debug_buf, sizeof(debug_buf),
                 "[DEBUG] FAIL: cmd_type=%u > COMMAND_STOP=%u\n",
                 cmd_type, COMMAND_STOP);
        uart_send_string_blocking(debug_buf);
#endif
        send_response(RESP_ERR_INVALID_COMMAND, cmd_id);
        return false;
    }

#ifdef DEBUG
        uart_send_string_blocking("[DEBUG] Validation passed!\n");
#endif

        // Guardar información del comando
        command->cmd.type = cmd_type;
        command->cmd.id = cmd_id;
        command->cmd.intensity = cmd_intensity;
    

    // Enviar ACK
    send_response(RESP_ACK, cmd_id);

#ifdef DEBUG
    char debug_msg[80];
    snprintf(debug_msg, sizeof(debug_msg), "[RX] COMMAND: %s \t INTENSITY: %s \t (ID:%u) \n",
             get_command_name(cmd_type), get_command_intensity(cmd_intensity), cmd_id);
    uart_send_string_blocking(debug_msg);
#endif

    return true;
}

/**
 * @brief Envía respuesta en formato: S:<RESP_TYPE>:<COMMAND_ID>:E
 * Ejemplo: "S:0:123:E" (ACK para comando con ID 123)
 */
static void send_response(uart_resp_id_t resp_type, uint16_t cmd_id)
{
    char response[20];
    snprintf(response, sizeof(response), "S:%d:%u:E", resp_type, cmd_id);
    uart_send_string_blocking(response);

#ifdef DEBUG
    char debug_msg[60];
    snprintf(debug_msg, sizeof(debug_msg), "[TX] RESP: %s (ID:%u)\n",
             get_response_name(resp_type), cmd_id);
    uart_send_string_blocking(debug_msg);
#endif
}

void clean_buffer(void)
{
    char clean_buffer[UART_RX_BUFFER_SIZE];
    strncpy(clean_buffer, (const char *)rx_buffer, rx_write_index);
    clean_buffer[rx_write_index] = '\0';
}

#ifdef DEBUG
/**
 * @brief Convierte tipo de comando a string para debug
 */
static const char *get_command_name(uint8_t cmd_type)
{
    switch (cmd_type)
    {
    case COMMAND_MOVE_FORWARD:
        return "FORWARD";
    case COMMAND_MOVE_BACKWARDS:
        return "BACKWARDS";
    case COMMAND_MOVE_LEFT:
        return "LEFT";
    case COMMAND_MOVE_RIGHT:
        return "RIGHT";
    case COMMAND_STOP:
        return "STOP";
    default:
        return "UNKNOWN";
    }
}

static const char *get_command_intensity(uint8_t cmd_intensity)
{
    switch (cmd_intensity)
    {
    case INTENSITY_LOW:
        return "INTENSITY_LOW";
    case INTENSITY_MEDIUM:
        return "INTENSITY_MEDIUM";
    case INTENSITY_HIGH:
        return "INTENSITY_HIGH";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Convierte tipo de respuesta a string para debug
 */
static const char *get_response_name(uart_resp_id_t resp_type)
{
    switch (resp_type)
    {
    case RESP_ACK:
        return "ACK";
    case RESP_READY:
        return "READY";
    case RESP_NACK:
        return "NACK";
    case RESP_ERR_INVALID_COMMAND:
        return "ERR_INVALID_COMMAND";
    case RESP_ERR_INVALID_PARAMS:
        return "ERR_INVALID_PARAMS";
    default:
        return "UNKNOWN";
    }
}
#endif