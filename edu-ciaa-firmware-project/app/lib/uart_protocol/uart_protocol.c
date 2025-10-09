#include "uart_protocol.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* === FUNCIONES DE MAPEO === */

uart_cmd_id_t uart_cmd_from_str(const char *code)
{
    if (code == NULL)
        return UART_CMD_UNKNOWN;

    /* Necesitamos al menos 2 caracteres */
    if (code[0] == '\0' || code[1] == '\0')
        return UART_CMD_UNKNOWN;

    char a = (char)toupper((unsigned char)code[0]);
    char b = (char)toupper((unsigned char)code[1]);

    if (a == 'M' && b == 'V')
        return UART_CMD_MV;
    if (a == 'S' && b == 'T')
        return UART_CMD_ST;
    if (a == 'G' && b == 'T')
        return UART_CMD_GT;
    if (a == 'S' && b == 'F')
        return UART_CMD_SF;
    if (a == 'S' && b == 'B')
        return UART_CMD_SB;
    if (a == 'S' && b == 'L')
        return UART_CMD_SL;
    if (a == 'S' && b == 'R')
        return UART_CMD_SR;

    return UART_CMD_UNKNOWN;
}

const char *uart_cmd_to_str(uart_cmd_id_t cmd)
{
    switch (cmd)
    {
    case UART_CMD_MV:
        return "MV";
    case UART_CMD_ST:
        return "ST";
    case UART_CMD_GT:
        return "GT";
    case UART_CMD_SF:
        return "SF";
    case UART_CMD_SB:
        return "SB";
    case UART_CMD_SL:
        return "SL";
    case UART_CMD_SR:
        return "SR";
    default:
        return "UNK";
    }
}

const char *uart_resp_to_str(uart_resp_id_t resp)
{
    switch (resp)
    {
    case UART_RESP_OK:
        return "OK";
    case UART_RESP_NACK:
        return "NACK";
    case UART_RESP_ERR_INVALID_COMMAND:
        return "ERR:INVALID_COMMAND";
    case UART_RESP_ERR_INVALID_PARAMS:
        return "ERR:INVALID_PARAMS";
    case UART_RESP_ERR_BUFFER_FULL:
        return "ERR:BUFFER_FULL";
    case UART_RESP_TELEMETRY:
        return "TEL";
    default:
        return "ERR:UNKNOWN";
    }
}

rover_cmd_type_t uart_cmd_to_rover_cmd(uart_cmd_id_t uart_cmd, const double *params, uint8_t param_count)
{
    switch (uart_cmd)
    {
    case UART_CMD_ST:
        return ROVER_CMD_STOP;

    case UART_CMD_SF:
        return ROVER_CMD_MOVE_FORWARD;

    case UART_CMD_SB:
        return ROVER_CMD_MOVE_BACKWARDS;

    case UART_CMD_SL:
        return ROVER_CMD_MOVE_LEFT;

    case UART_CMD_SR:
        return ROVER_CMD_MOVE_RIGHT;

    case UART_CMD_MV:
        /* Para MV, analizamos los parámetros para determinar el movimiento */
        if (param_count >= 2)
        {
            double left = params[0];
            double right = params[1];

            if (left == 0.0 && right == 0.0)
            {
                return ROVER_CMD_STOP;
            }
            else if (left > 0.0 && right > 0.0)
            {
                return (left == right) ? ROVER_CMD_MOVE_FORWARD : ROVER_CMD_CUSTOM_MOVE;
            }
            else if (left < 0.0 && right < 0.0)
            {
                return (left == right) ? ROVER_CMD_MOVE_BACKWARDS : ROVER_CMD_CUSTOM_MOVE;
            }
            else
            {
                /* Giro: una rueda positiva, otra negativa o cero */
                if (left > right)
                {
                    return ROVER_CMD_MOVE_RIGHT;
                }
                else
                {
                    return ROVER_CMD_MOVE_LEFT;
                }
            }
        }
        return ROVER_CMD_CUSTOM_MOVE;

    case UART_CMD_GT:
        return ROVER_CMD_GET_TELEMETRY;

    default:
        return ROVER_CMD_STOP;
    }
}

/* === FUNCIONES DE PARSING DE PARÁMETROS === */

static int parse_params_list(const char *s, double *out, int max_params)
{
    int count = 0;
    if (!s || !out)
        return 0;

    const char *p = s;
    char *end;

    while (count < max_params && *p)
    {
        /* Saltar espacios */
        while (*p && isspace((unsigned char)*p))
            p++;
        if (!*p)
            break;

        /* Parsear número */
        out[count] = strtod(p, &end);
        if (p == end)
            break; /* No se pudo convertir */

        count++;
        p = end;

        /* Saltar espacios y coma */
        while (*p && isspace((unsigned char)*p))
            p++;
        if (*p == ',')
        {
            p++;
        }
        else if (*p != '\0')
        {
            break; /* Carácter inesperado */
        }
    }

    return count;
}

/* === FUNCIONES DE PARSING === */

int uart_parse_frame_to_cmd(const char *frame, data_cmd_t *out)
{
    if (!frame || !out)
        return -2;

    /* Limpiar estructura de salida */
    memset(out, 0, sizeof(*out));

    /* Saltar espacios iniciales */
    while (*frame && isspace((unsigned char)*frame))
        frame++;

    /* Necesita al menos 2 caracteres para el comando */
    if (!frame[0] || !frame[1])
        return -1;

    /* Identificar comando */
    uart_cmd_id_t cmd_id = uart_cmd_from_str(frame);
    if (cmd_id == UART_CMD_UNKNOWN)
        return -1;

    out->uart_cmd = cmd_id;

    /* Buscar parámetros después de "XY" o "XY:" */
    const char *params = frame + 2;
    if (*params == ':')
        params++;

    /* Parsear parámetros según el comando */
    switch (cmd_id)
    {
    case UART_CMD_MV:
        /* MV: espera 2 parámetros (vel_izq, vel_der) */
        out->total_params = parse_params_list(params, out->params, 2);
        if (out->total_params < 2)
            return -2;
        break;

    case UART_CMD_SF:
    case UART_CMD_SB:
    case UART_CMD_SL:
    case UART_CMD_SR:
        /* Comandos direccionales: 1 parámetro opcional (velocidad) */
        out->total_params = parse_params_list(params, out->params, 1);
        if (out->total_params == 0)
        {
            /* Sin parámetros, usar velocidad por defecto */
            out->params[0] = 255.0;
            out->total_params = 1;
        }
        break;

    case UART_CMD_ST:
    case UART_CMD_GT:
        /* Sin parámetros */
        out->total_params = 0;
        break;

    default:
        return -1;
    }

    /* Mapear a comando del rover */
    out->rover_cmd = uart_cmd_to_rover_cmd(cmd_id, out->params, out->total_params);

    return 0;
}

int uart_cmd_to_frame(const data_cmd_t *cmd, char *frame, size_t max_len)
{
    if (!cmd || !frame || max_len < 8)
        return -1;

    const char *cmd_str = uart_cmd_to_str(cmd->uart_cmd);
    int len = 0;

    /* Agregar inicio de trama */
    frame[len++] = UART_FRAME_START;

    /* Agregar comando */
    int cmd_len = strlen(cmd_str);
    if (len + cmd_len >= max_len - 3)
        return -1; /* -3 para ':', 'E' y '\0' */

    strcpy(&frame[len], cmd_str);
    len += cmd_len;

    /* Agregar parámetros si los hay */
    if (cmd->total_params > 0)
    {
        frame[len++] = ':';

        for (uint8_t i = 0; i < cmd->total_params; i++)
        {
            if (i > 0)
            {
                if (len >= max_len - 2)
                    return -1;
                frame[len++] = ',';
            }

            int param_len = snprintf(&frame[len], max_len - len - 2, "%.1f", cmd->params[i]);
            if (param_len < 0 || len + param_len >= max_len - 2)
                return -1;
            len += param_len;
        }
    }

    /* Agregar fin de trama */
    if (len >= max_len - 1)
        return -1;
    frame[len++] = UART_FRAME_END;
    frame[len] = '\0';

    return len;
}

int uart_telemetry_to_frame(const telemetry_data_t *tel_data, char *frame, size_t max_len)
{
    if (!tel_data || !frame || max_len < 32)
        return -1;

    int len = snprintf(frame, max_len,
                       "%cTEL:%.2f,%.1f,%.1f,%.1f,%lu%c",
                       UART_FRAME_START,
                       tel_data->battery_voltage,
                       tel_data->left_wheel_speed,
                       tel_data->right_wheel_speed,
                       tel_data->temperature,
                       (unsigned long)tel_data->timestamp,
                       UART_FRAME_END);

    return (len > 0 && len < max_len) ? len : -1;
}

uart_resp_id_t uart_parse_response(const char *frame)
{
    if (!frame)
        return UART_RESP_NACK;

    /* Saltar espacios y marco inicial */
    while (*frame && (isspace((unsigned char)*frame) || *frame == UART_FRAME_START))
    {
        frame++;
    }

    if (strncmp(frame, "OK", 2) == 0)
        return UART_RESP_OK;
    if (strncmp(frame, "NACK", 4) == 0)
        return UART_RESP_NACK;
    if (strncmp(frame, "ERR:INVALID_COMMAND", 19) == 0)
        return UART_RESP_ERR_INVALID_COMMAND;
    if (strncmp(frame, "ERR:INVALID_PARAMS", 18) == 0)
        return UART_RESP_ERR_INVALID_PARAMS;
    if (strncmp(frame, "ERR:BUFFER_FULL", 15) == 0)
        return UART_RESP_ERR_BUFFER_FULL;
    if (strncmp(frame, "TEL:", 4) == 0)
        return UART_RESP_TELEMETRY;

    return UART_RESP_NACK;
}

/* === FUNCIONES DE BUFFER === */

void cmd_buffer_init(cmd_buffer_t *buf)
{
    if (!buf)
        return;

    memset(buf, 0, sizeof(*buf));
    buf->head = 0;
    buf->count = 0;
    buf->newest_id = 0;
}

int cmd_buffer_enqueue(cmd_buffer_t *buf, const data_cmd_t *cmd)
{
    if (!buf || !cmd)
        return -1;

    /* Calcular índice de escritura */
    uint8_t write_idx = (buf->head + buf->count) % UART_CMD_BUFFER_LEN;

    /* Si el buffer está lleno, sobrescribir el más antiguo */
    if (buf->count == UART_CMD_BUFFER_LEN)
    {
        buf->head = (buf->head + 1) % UART_CMD_BUFFER_LEN;
    }
    else
    {
        buf->count++;
    }

    /* Copiar comando y asignar ID */
    buf->buffer[write_idx] = *cmd;
    buf->buffer[write_idx].id = ++buf->newest_id;

    return 0;
}

int cmd_buffer_dequeue(cmd_buffer_t *buf, data_cmd_t *cmd)
{
    if (!buf || !cmd || buf->count == 0)
        return -1;

    /* Copiar comando del head */
    *cmd = buf->buffer[buf->head];

    /* Actualizar head y count */
    buf->head = (buf->head + 1) % UART_CMD_BUFFER_LEN;
    buf->count--;

    return 0;
}

bool cmd_buffer_is_empty(const cmd_buffer_t *buf)
{
    return (buf == NULL) || (buf->count == 0);
}

bool cmd_buffer_is_full(const cmd_buffer_t *buf)
{
    return (buf != NULL) && (buf->count == UART_CMD_BUFFER_LEN);
}

uint8_t cmd_buffer_count(const cmd_buffer_t *buf)
{
    return (buf != NULL) ? buf->count : 0;
}