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


#define ACK_AWAIT_MS 200 // Deben definirse los tiempos en la propia fase de pruebas
#define CMD_AWAIT_MS 1000
#define RESPONSE_LEN 13
#define CMD_PARAMS_LEN 10

const char *response_format = "%c:%d:%d:%c";
const char *cmd_format = "%c:%d:";

    /* IDs de respuestas/envíos desde EDU-CIAA */
    typedef enum {
        RESP_ACK = 0,
        RESP_READY,
        RESP_NACK,
        RESP_ERR_INVALID_COMMAND,
        RESP_ERR_INVALID_PARAMS,
        RESP_COUNT
    } uart_resp_id_t;

    typedef enum
    {
        CMD_MOVE_FORWARD = 0,
        CMD_MOVE_BACKWARDS,
        CMD_MOVE_LEFT,
        CMD_MOVE_RIGHT
    } rover_cmd_type_t;

    typedef struct
    {
        uint16_t id;
        rover_cmd_type_t cmd;
        double params[CMD_PARAMS_LEN]; // This is a estimate, we should see if it's less or more.
        uint8_t total_params;

    } data_cmd;
    
#endif