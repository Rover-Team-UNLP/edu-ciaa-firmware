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

/**
 * Leer!
 * 
 * El codigo usa un #define DEBUG que cambia muchos aspectos, entre ellos el parseo de 
 * comandos por uart. Para debuguear utiliza mascaras y tipos mas simples de tipear
 * para la persona que esta debugueando. 
 * 
 * Comentar el #define DEBUG de uart_comm.c cuando se quiera probar el codigo de micro a micro. 
 * 
 * La mascara para los uint8_t es %hhu, y para uint16_t es %hu.
 */


// Formato del comando ESP32 -> CIAA
// const char *cmd_format = "S:%hhu:%hhu:%hu:E";

// Formato de respuesta CIAA -> ESP32
// const char *response_format = "S:%d:%d:E";

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

/* Tipos de comando */
typedef enum
{
    COMMAND_MOVE_FORWARD = 0,
    COMMAND_MOVE_BACKWARDS,
    COMMAND_MOVE_LEFT,
    COMMAND_MOVE_RIGHT,
    COMMAND_STOP
} rover_cmd_type_t;

/* Intensidad del comando*/
typedef enum {
    INTENSITY_LOW = 0,
    INTENSITY_MEDIUM,
    INTENSITY_HIGH,
} rover_cmd_intensity_t;

typedef struct
{
    uint16_t id;
    rover_cmd_type_t type;
    rover_cmd_intensity_t intensity;
} data_cmd;

#endif