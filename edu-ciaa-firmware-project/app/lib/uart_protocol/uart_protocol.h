#ifndef UART_PROTOCOL_H_
#define UART_PROTOCOL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

/* Configuración del protocolo */
#define UART_FRAME_START 'S'
#define UART_FRAME_END 'E'
#define UART_MAX_FRAME_SIZE 64
#define UART_CMD_PARAMS_LEN 10
#define UART_CMD_BUFFER_LEN 10

    /* IDs de comandos recibidos por UART (desde ESP32) */
    typedef enum
    {
        UART_CMD_UNKNOWN = 0,
        UART_CMD_MV, /* Move: "MV" params: vel_izq,vel_der */
        UART_CMD_ST, /* Stop: "ST" no params */
        UART_CMD_GT, /* Get Telemetry: "GT" no params */
        UART_CMD_SF, /* Set Forward: "SF" params: vel */
        UART_CMD_SB, /* Set Backward: "SB" params: vel */
        UART_CMD_SL, /* Set Left: "SL" params: vel */
        UART_CMD_SR, /* Set Right: "SR" params: vel */
        UART_CMD_COUNT
    } uart_cmd_id_t;

    /* Tipos de comandos del rover (implementación interna) */
    typedef enum
    {
        ROVER_CMD_STOP = 0,
        ROVER_CMD_MOVE_FORWARD,
        ROVER_CMD_MOVE_BACKWARDS,
        ROVER_CMD_MOVE_LEFT,
        ROVER_CMD_MOVE_RIGHT,
        ROVER_CMD_CUSTOM_MOVE, /* Para velocidades específicas por rueda */
        ROVER_CMD_GET_TELEMETRY,
        ROVER_CMD_COUNT
    } rover_cmd_type_t;

    /* IDs de respuestas/envíos */
    typedef enum
    {
        UART_RESP_OK = 0,              /* "OK" - ACK genérico */
        UART_RESP_NACK,                /* "NACK" - NACK genérico */
        UART_RESP_ERR_INVALID_COMMAND, /* "ERR:INVALID_COMMAND" */
        UART_RESP_ERR_INVALID_PARAMS,  /* "ERR:INVALID_PARAMS" */
        UART_RESP_ERR_BUFFER_FULL,     /* "ERR:BUFFER_FULL" */
        UART_RESP_TELEMETRY,           /* "TEL:..." - datos de telemetría */
        UART_RESP_COUNT
    } uart_resp_id_t;

    /* Comando individual */
    typedef struct
    {
        uint16_t id;                        /* ID único del comando */
        uart_cmd_id_t uart_cmd;             /* Comando UART original */
        rover_cmd_type_t rover_cmd;         /* Comando interpretado para el rover */
        double params[UART_CMD_PARAMS_LEN]; /* Parámetros (máximo 10) */
        uint8_t total_params;               /* Cantidad de parámetros */
    } data_cmd_t;

    /* Buffer circular de comandos */
    typedef struct
    {
        data_cmd_t buffer[UART_CMD_BUFFER_LEN]; /* Buffer (capacidad 10) */
        uint16_t newest_id;                     /* ID más reciente */
        uint8_t head;                           /* Índice del primer elemento */
        uint8_t count;                          /* Cantidad actual de comandos */
    } cmd_buffer_t;

    /* Estructura para datos de telemetría */
    typedef struct
    {
        double battery_voltage;   /* Voltaje de batería */
        double left_wheel_speed;  /* Velocidad rueda izquierda */
        double right_wheel_speed; /* Velocidad rueda derecha */
        double temperature;       /* Temperatura del sistema */
        uint32_t timestamp;       /* Timestamp del dato */
    } telemetry_data_t;

/* Macros de compatibilidad */
#define ACK (UART_RESP_OK)
#define NACK (UART_RESP_NACK)

    /* === FUNCIONES DE PARSING === */

    /**
     * @brief Parsea una trama UART cruda a estructura data_cmd_t
     * @param frame Contenido entre 'S' y 'E' (ej: "MV:255,-255")
     * @param out Puntero a estructura de salida
     * @return 0 en éxito, -1 comando desconocido, -2 parámetros inválidos
     */
    int uart_parse_frame_to_cmd(const char *frame, data_cmd_t *out);

    /**
     * @brief Convierte una estructura data_cmd_t a trama UART
     * @param cmd Comando a serializar
     * @param frame Buffer de salida (mínimo UART_MAX_FRAME_SIZE)
     * @param max_len Tamaño máximo del buffer
     * @return Longitud de la trama generada, -1 en error
     */
    int uart_cmd_to_frame(const data_cmd_t *cmd, char *frame, size_t max_len);

    /**
     * @brief Convierte datos de telemetría a trama UART
     * @param tel_data Datos de telemetría
     * @param frame Buffer de salida
     * @param max_len Tamaño máximo del buffer
     * @return Longitud de la trama generada, -1 en error
     */
    int uart_telemetry_to_frame(const telemetry_data_t *tel_data, char *frame, size_t max_len);

    /**
     * @brief Parsea una respuesta UART a ID de respuesta
     * @param frame Contenido de respuesta
     * @return ID de respuesta correspondiente
     */
    uart_resp_id_t uart_parse_response(const char *frame);

    /* === FUNCIONES DE MAPEO === */

    /**
     * @brief Convierte código de texto a ID de comando UART
     * @param code Código de comando (ej: "MV", "ST")
     * @return ID de comando correspondiente
     */
    uart_cmd_id_t uart_cmd_from_str(const char *code);

    /**
     * @brief Convierte ID de comando UART a texto
     * @param cmd ID de comando
     * @return Representación textual del comando
     */
    const char *uart_cmd_to_str(uart_cmd_id_t cmd);

    /**
     * @brief Convierte ID de respuesta a texto
     * @param resp ID de respuesta
     * @return Representación textual de la respuesta
     */
    const char *uart_resp_to_str(uart_resp_id_t resp);

    /**
     * @brief Mapea comando UART a comando del rover
     * @param uart_cmd Comando UART
     * @param params Parámetros del comando
     * @param param_count Cantidad de parámetros
     * @return Tipo de comando del rover correspondiente
     */
    rover_cmd_type_t uart_cmd_to_rover_cmd(uart_cmd_id_t uart_cmd, const double *params, uint8_t param_count);

    /* === FUNCIONES DE BUFFER === */

    /**
     * @brief Inicializa un buffer de comandos
     * @param buf Buffer a inicializar
     */
    void cmd_buffer_init(cmd_buffer_t *buf);

    /**
     * @brief Encola un comando en el buffer circular
     * @param buf Buffer de comandos
     * @param cmd Comando a encolar
     * @return 0 en éxito, -1 en error
     */
    int cmd_buffer_enqueue(cmd_buffer_t *buf, const data_cmd_t *cmd);

    /**
     * @brief Desencola un comando del buffer
     * @param buf Buffer de comandos
     * @param cmd Puntero donde almacenar el comando
     * @return 0 en éxito, -1 si buffer vacío
     */
    int cmd_buffer_dequeue(cmd_buffer_t *buf, data_cmd_t *cmd);

    /**
     * @brief Verifica si el buffer está vacío
     * @param buf Buffer de comandos
     * @return true si está vacío, false en caso contrario
     */
    bool cmd_buffer_is_empty(const cmd_buffer_t *buf);

    /**
     * @brief Verifica si el buffer está lleno
     * @param buf Buffer de comandos
     * @return true si está lleno, false en caso contrario
     */
    bool cmd_buffer_is_full(const cmd_buffer_t *buf);

    /**
     * @brief Obtiene la cantidad de comandos en el buffer
     * @param buf Buffer de comandos
     * @return Cantidad de comandos
     */
    uint8_t cmd_buffer_count(const cmd_buffer_t *buf);

#ifdef __cplusplus
}
#endif

#endif /* UART_PROTOCOL_H_ */