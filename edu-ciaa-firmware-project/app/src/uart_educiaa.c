#include "uart_educiaa.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* === FUNCIONES PRINCIPALES === */

int uart_educiaa_init(uart_educiaa_t *uart_ctx)
{
    if (!uart_ctx)
        return -1;

    /* Limpiar contexto */
    memset(uart_ctx, 0, sizeof(*uart_ctx));

    /* Inicializar estado del parser */
    uart_ctx->state = UART_STATE_WAITING_START;
    uart_ctx->rx_index = 0;

    /* Inicializar buffer de comandos */
    cmd_buffer_init(&uart_ctx->cmd_buffer);

    /* Inicializar hardware UART */
    Board_Debug_Init(); /* Inicializa UART a 115200 bps */

    /* Enviar mensaje de inicio */
    uart_educiaa_send_line("EDU-CIAA UART initialized");

    return 0;
}

int uart_educiaa_process(uart_educiaa_t *uart_ctx)
{
    if (!uart_ctx)
        return -1;

    int commands_processed = 0;
    int ch;

    /* Procesar todos los caracteres disponibles */
    while ((ch = Board_UARTGetChar()) != EOF)
    {
        /* Verificar overflow del buffer de recepción */
        if (uart_ctx->rx_index >= EDUCIAA_UART_RX_BUFFER_SIZE - 1)
        {
            /* Buffer overflow: resetear estado */
            uart_ctx->state = UART_STATE_WAITING_START;
            uart_ctx->rx_index = 0;
            uart_ctx->buffer_overflows++;
            continue;
        }

        switch (uart_ctx->state)
        {
        case UART_STATE_WAITING_START:
            if (ch == UART_FRAME_START)
            {
                uart_ctx->state = UART_STATE_RECEIVING;
                uart_ctx->rx_index = 0;
            }
            break;

        case UART_STATE_RECEIVING:
            if (ch == UART_FRAME_END)
            {
                /* Trama completa recibida */
                uart_ctx->rx_buffer[uart_ctx->rx_index] = '\0';
                uart_ctx->state = UART_STATE_FRAME_READY;
                uart_ctx->frames_received++;

                /* Procesar la trama */
                data_cmd_t cmd;
                int parse_result = uart_parse_frame_to_cmd(uart_ctx->rx_buffer, &cmd);

                if (parse_result == 0)
                {
                    /* Comando válido: encolar */
                    if (cmd_buffer_enqueue(&uart_ctx->cmd_buffer, &cmd) == 0)
                    {
                        commands_processed++;
                        uart_educiaa_send_response(uart_ctx, UART_RESP_OK);
                    }
                    else
                    {
                        uart_educiaa_send_response(uart_ctx, UART_RESP_ERR_BUFFER_FULL);
                    }
                }
                else
                {
                    /* Error de parsing */
                    uart_ctx->parse_errors++;
                    if (parse_result == -1)
                    {
                        uart_educiaa_send_response(uart_ctx, UART_RESP_ERR_INVALID_COMMAND);
                    }
                    else
                    {
                        uart_educiaa_send_response(uart_ctx, UART_RESP_ERR_INVALID_PARAMS);
                    }
                }

                /* Resetear estado para la próxima trama */
                uart_ctx->state = UART_STATE_WAITING_START;
                uart_ctx->rx_index = 0;
            }
            else if (ch == UART_FRAME_START)
            {
                /* Nueva trama antes de completar la anterior */
                uart_ctx->rx_index = 0;
            }
            else
            {
                /* Carácter de datos */
                uart_ctx->rx_buffer[uart_ctx->rx_index++] = (char)ch;
            }
            break;

        case UART_STATE_FRAME_READY:
            /* Estado transitorio, no debería llegar aquí */
            uart_ctx->state = UART_STATE_WAITING_START;
            break;
        }
    }

    return commands_processed;
}

int uart_educiaa_send_response(uart_educiaa_t *uart_ctx, uart_resp_id_t resp_id)
{
    if (!uart_ctx)
        return -1;

    char response[32];
    int len = snprintf(response, sizeof(response), "%c%s%c",
                       UART_FRAME_START,
                       uart_resp_to_str(resp_id),
                       UART_FRAME_END);

    if (len > 0 && len < sizeof(response))
    {
        Board_UARTPutSTR(response);
        uart_ctx->frames_sent++;
        return 0;
    }

    return -1;
}

int uart_educiaa_send_telemetry(uart_educiaa_t *uart_ctx, const telemetry_data_t *tel_data)
{
    if (!uart_ctx || !tel_data)
        return -1;

    char frame[UART_MAX_FRAME_SIZE];
    int len = uart_telemetry_to_frame(tel_data, frame, sizeof(frame));

    if (len > 0)
    {
        Board_UARTPutSTR(frame);
        uart_ctx->frames_sent++;
        return 0;
    }

    return -1;
}

int uart_educiaa_get_command(uart_educiaa_t *uart_ctx, data_cmd_t *cmd)
{
    if (!uart_ctx || !cmd)
        return -1;

    return cmd_buffer_dequeue(&uart_ctx->cmd_buffer, cmd);
}

bool uart_educiaa_has_commands(const uart_educiaa_t *uart_ctx)
{
    if (!uart_ctx)
        return false;

    return !cmd_buffer_is_empty(&uart_ctx->cmd_buffer);
}

void uart_educiaa_get_stats(const uart_educiaa_t *uart_ctx,
                            uint32_t *frames_rx, uint32_t *frames_tx,
                            uint32_t *errors, uint32_t *overflows)
{
    if (!uart_ctx)
        return;

    if (frames_rx)
        *frames_rx = uart_ctx->frames_received;
    if (frames_tx)
        *frames_tx = uart_ctx->frames_sent;
    if (errors)
        *errors = uart_ctx->parse_errors;
    if (overflows)
        *overflows = uart_ctx->buffer_overflows;
}

void uart_educiaa_reset_stats(uart_educiaa_t *uart_ctx)
{
    if (!uart_ctx)
        return;

    uart_ctx->frames_received = 0;
    uart_ctx->frames_sent = 0;
    uart_ctx->parse_errors = 0;
    uart_ctx->buffer_overflows = 0;
}

/* === FUNCIONES DE UTILIDAD === */

void uart_educiaa_send_string(const char *str)
{
    if (str)
    {
        Board_UARTPutSTR(str);
    }
}

void uart_educiaa_send_line(const char *str)
{
    if (str)
    {
        Board_UARTPutSTR(str);
        Board_UARTPutSTR("\r\n");
    }
}

void uart_educiaa_printf(const char *format, ...)
{
    if (!format)
        return;

    char buffer[256];
    va_list args;

    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len > 0 && len < sizeof(buffer))
    {
        Board_UARTPutSTR(buffer);
    }
}