#ifndef UART_COMMANDS_H_
#define UART_COMMANDS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* IDs de comandos recibidos por UART (desde ESP32) */
typedef enum {
    CMD_UNKNOWN = 0,
    CMD_MV,    // Move:  "MV"  params: vel_izq,vel_der  
    CMD_ST,    // Stop:  "ST"  no params                    
    CMD_GT,    // Get Telemetry: "GT" no params             
    CMD_COUNT
} uart_cmd_id_t;

/* IDs de respuestas/envíos desde EDU-CIAA */
typedef enum {
    RESP_OK = 0,                 /* ACK genérico */
    RESP_NACK,                   /* NACK genérico */
    RESP_ERR_INVALID_COMMAND,    /* ERR:INVALID_COMMAND */
    RESP_ERR_INVALID_PARAMS,     /* ERR:INVALID_PARAMS */
    RESP_COUNT
} uart_resp_id_t;

/* Macros legadas */
#define ACK  (RESP_OK)
#define NACK (RESP_NACK)
#define RESP_EOF (RESP_COUNT) /* marcador reservado, evitar usar EOF por colisión con stdio */

uart_cmd_id_t uart_cmd_from_str(const char *code);

const char *uart_cmd_to_str(uart_cmd_id_t cmd);

const char *uart_resp_to_str(uart_resp_id_t resp);

#ifdef __cplusplus
}
#endif

#endif /* UART_COMMANDS_H_ */