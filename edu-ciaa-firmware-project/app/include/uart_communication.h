#include <stdlib.h>
#include "board.h"

#define CMD_PARAMS_LEN 10
#define CMD_BUFFER_LEN 10

typedef enum {
    CMD_MOVE_FORWARD = 0,
    CMD_MOVE_BACKWARDS,
    CMD_MOVE_LEFT,
    CMD_MOVE_RIGHT
} rover_cmd_type_t;

// Comando individual
typedef struct {
    uint16_t id;                        // ID único del comando
    rover_cmd_type_t cmd;               // Tipo de comando
    double params[CMD_PARAMS_LEN];      // Parámetros (máximo 10)
    uint8_t total_params;               // Cantidad de parámetros
} data_cmd_t;

// Buffer circular de comandos
typedef struct {
    data_cmd_t buffer[CMD_BUFFER_LEN];    // Buffer (capacidad 10)
    uint16_t newest_id;                 // ID más reciente
    uint8_t count;                      // Cantidad actual de comandos
} cmd_buffer_t;