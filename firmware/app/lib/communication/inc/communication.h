/* ======================================
- File: communication.h
- Description: Header file with macros and structs for ESP32-CIAA communication
- Author/s: @JuanCruzFerreiraM, @taciano-pacchialat
- Last-update: 2025-10-28
- ====================================== */
#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdlib.h>
#include <stdint.h>

#define RESPONSE_LEN 13
#define COMMAND_PARAMS_LEN 10

// const char *response_format = "S:%d:%d:E";
// const char *cmd_format = "S:%d:%d:E";

/* IDs de respuestas/envíos desde EDU-CIAA */
typedef enum
{
    RESP_ACK = 0,
    RESP_READY,
    RESP_NACK,
    RESP_ERR_INVALID_COMMAND,
    RESP_ERR_INVALID_PARAMS,
    RESP_COUNT
} uart_resp_id_t;

typedef enum
{
    COMMAND_MOVE_FORWARD = 0,
    COMMAND_MOVE_BACKWARDS,
    COMMAND_MOVE_LEFT,
    COMMAND_MOVE_RIGHT,
    COMMAND_STOP
} rover_cmd_type_t;

typedef struct
{
    uint16_t id;
    rover_cmd_type_t cmd;
    double params[COMMAND_PARAMS_LEN]; // This is a estimate, we should see if it's less or more.
    uint8_t total_params;
} data_cmd;

#endif