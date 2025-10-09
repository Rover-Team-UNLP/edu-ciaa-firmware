#ifndef UART_EDUCIAA_H_
#define UART_EDUCIAA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "uart_protocol.h"
#include "board.h"

/* Configuración específica de la EDU-CIAA */
#define EDUCIAA_UART_RX_BUFFER_SIZE 128
#define EDUCIAA_UART_TX_BUFFER_SIZE 128
#define EDUCIAA_UART_FRAME_TIMEOUT_MS 100

/* Estados del parser de tramas */
typedef enum {
    UART_STATE_WAITING_START = 0,
    UART_STATE_RECEIVING,
    UART_STATE_FRAME_READY
} uart_parser_state_t;

/* Estructura para manejo de la UART de la EDU-CIAA */
typedef struct {
    /* Buffers de recepción */
    char rx_buffer[EDUCIAA_UART_RX_BUFFER_SIZE];
    uint16_t rx_index;
    
    /* Estado del parser */
    uart_parser_state_t state;
    
    /* Buffer de comandos */
    cmd_buffer_t cmd_buffer;
    
    /* Estadísticas */
    uint32_t frames_received;
    uint32_t frames_sent;
    uint32_t parse_errors;
    uint32_t buffer_overflows;
} uart_educiaa_t;

/* === FUNCIONES PRINCIPALES === */

/**
 * @brief Inicializa la UART de la EDU-CIAA
 * @param uart_ctx Contexto de la UART
 * @return 0 en éxito, -1 en error
 */
int uart_educiaa_init(uart_educiaa_t *uart_ctx);

/**
 * @brief Procesa datos recibidos por UART (llamar en loop principal)
 * @param uart_ctx Contexto de la UART
 * @return Número de comandos nuevos procesados
 */
int uart_educiaa_process(uart_educiaa_t *uart_ctx);

/**
 * @brief Envía una respuesta por UART
 * @param uart_ctx Contexto de la UART
 * @param resp_id ID de respuesta a enviar
 * @return 0 en éxito, -1 en error
 */
int uart_educiaa_send_response(uart_educiaa_t *uart_ctx, uart_resp_id_t resp_id);

/**
 * @brief Envía datos de telemetría por UART
 * @param uart_ctx Contexto de la UART
 * @param tel_data Datos de telemetría
 * @return 0 en éxito, -1 en error
 */
int uart_educiaa_send_telemetry(uart_educiaa_t *uart_ctx, const telemetry_data_t *tel_data);

/**
 * @brief Obtiene el próximo comando del buffer
 * @param uart_ctx Contexto de la UART
 * @param cmd Puntero donde almacenar el comando
 * @return 0 en éxito, -1 si no hay comandos
 */
int uart_educiaa_get_command(uart_educiaa_t *uart_ctx, data_cmd_t *cmd);

/**
 * @brief Verifica si hay comandos pendientes
 * @param uart_ctx Contexto de la UART
 * @return true si hay comandos, false en caso contrario
 */
bool uart_educiaa_has_commands(const uart_educiaa_t *uart_ctx);

/**
 * @brief Obtiene estadísticas de la UART
 * @param uart_ctx Contexto de la UART
 * @param frames_rx Puntero para recibir número de tramas recibidas (puede ser NULL)
 * @param frames_tx Puntero para recibir número de tramas enviadas (puede ser NULL)
 * @param errors Puntero para recibir número de errores (puede ser NULL)
 * @param overflows Puntero para recibir número de overflows (puede ser NULL)
 */
void uart_educiaa_get_stats(const uart_educiaa_t *uart_ctx, 
                            uint32_t *frames_rx, uint32_t *frames_tx, 
                            uint32_t *errors, uint32_t *overflows);

/**
 * @brief Resetea las estadísticas de la UART
 * @param uart_ctx Contexto de la UART
 */
void uart_educiaa_reset_stats(uart_educiaa_t *uart_ctx);

/* === FUNCIONES DE UTILIDAD === */

/**
 * @brief Envía una cadena directamente por UART (para debug)
 * @param str Cadena a enviar
 */
void uart_educiaa_send_string(const char *str);

/**
 * @brief Envía una cadena con salto de línea por UART
 * @param str Cadena a enviar
 */
void uart_educiaa_send_line(const char *str);

/**
 * @brief Envía datos formateados por UART (como printf)
 * @param format Formato de la cadena
 * @param ... Argumentos variables
 */
void uart_educiaa_printf(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* UART_EDUCIAA_H_ */