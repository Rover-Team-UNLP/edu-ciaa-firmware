#include "board.h"
#include "uart_educiaa.h"
#include "uart_protocol.h"
#include <stdio.h>

/* Contexto global de la UART */
static uart_educiaa_t g_uart_ctx;

/* Datos de telemetría simulados */
static telemetry_data_t g_telemetry = {
    .battery_voltage = 12.5,
    .left_wheel_speed = 0.0,
    .right_wheel_speed = 0.0,
    .temperature = 25.5,
    .timestamp = 0};

/* Contador para envío periódico de telemetría */
static uint32_t g_telemetry_counter = 0;
#define TELEMETRY_PERIOD 10000 /* Enviar telemetría cada 10000 loops */

/* Función para procesar comandos del rover */
static void process_rover_command(const data_cmd_t *cmd)
{
    uart_educiaa_printf("Processing command ID=%u, Type=%s, Rover=%d\r\n",
                        cmd->id, uart_cmd_to_str(cmd->uart_cmd), cmd->rover_cmd);

    switch (cmd->rover_cmd)
    {
    case ROVER_CMD_STOP:
        uart_educiaa_printf("ROVER: STOP\r\n");
        g_telemetry.left_wheel_speed = 0.0;
        g_telemetry.right_wheel_speed = 0.0;
        break;

    case ROVER_CMD_MOVE_FORWARD:
        if (cmd->total_params > 0)
        {
            uart_educiaa_printf("ROVER: FORWARD speed=%.1f\r\n", cmd->params[0]);
            g_telemetry.left_wheel_speed = cmd->params[0];
            g_telemetry.right_wheel_speed = cmd->params[0];
        }
        else
        {
            uart_educiaa_printf("ROVER: FORWARD default speed\r\n");
            g_telemetry.left_wheel_speed = 255.0;
            g_telemetry.right_wheel_speed = 255.0;
        }
        break;

    case ROVER_CMD_MOVE_BACKWARDS:
        if (cmd->total_params > 0)
        {
            uart_educiaa_printf("ROVER: BACKWARD speed=%.1f\r\n", cmd->params[0]);
            g_telemetry.left_wheel_speed = -cmd->params[0];
            g_telemetry.right_wheel_speed = -cmd->params[0];
        }
        else
        {
            uart_educiaa_printf("ROVER: BACKWARD default speed\r\n");
            g_telemetry.left_wheel_speed = -255.0;
            g_telemetry.right_wheel_speed = -255.0;
        }
        break;

    case ROVER_CMD_MOVE_LEFT:
        if (cmd->total_params > 0)
        {
            uart_educiaa_printf("ROVER: LEFT speed=%.1f\r\n", cmd->params[0]);
            g_telemetry.left_wheel_speed = -cmd->params[0] * 0.5;
            g_telemetry.right_wheel_speed = cmd->params[0];
        }
        else
        {
            uart_educiaa_printf("ROVER: LEFT default speed\r\n");
            g_telemetry.left_wheel_speed = -127.0;
            g_telemetry.right_wheel_speed = 255.0;
        }
        break;

    case ROVER_CMD_MOVE_RIGHT:
        if (cmd->total_params > 0)
        {
            uart_educiaa_printf("ROVER: RIGHT speed=%.1f\r\n", cmd->params[0]);
            g_telemetry.left_wheel_speed = cmd->params[0];
            g_telemetry.right_wheel_speed = -cmd->params[0] * 0.5;
        }
        else
        {
            uart_educiaa_printf("ROVER: RIGHT default speed\r\n");
            g_telemetry.left_wheel_speed = 255.0;
            g_telemetry.right_wheel_speed = -127.0;
        }
        break;

    case ROVER_CMD_CUSTOM_MOVE:
        if (cmd->total_params >= 2)
        {
            uart_educiaa_printf("ROVER: CUSTOM left=%.1f right=%.1f\r\n",
                                cmd->params[0], cmd->params[1]);
            g_telemetry.left_wheel_speed = cmd->params[0];
            g_telemetry.right_wheel_speed = cmd->params[1];
        }
        break;

    case ROVER_CMD_GET_TELEMETRY:
        uart_educiaa_printf("ROVER: GET_TELEMETRY\r\n");
        g_telemetry.timestamp++;
        uart_educiaa_send_telemetry(&g_uart_ctx, &g_telemetry);
        break;

    default:
        uart_educiaa_printf("ROVER: Unknown command\r\n");
        break;
    }

    /* Aquí implementarías el control real de motores, sensores, etc. */
}

int main(void)
{
    /* Inicialización del sistema */
    Board_Init();

    /* Inicializar UART */
    if (uart_educiaa_init(&g_uart_ctx) != 0)
    {
        /* Error crítico */
        return -1;
    }

    uart_educiaa_send_line("=== EDU-CIAA Rover Control System ===");
    uart_educiaa_send_line("Ready to receive commands from ESP32");
    uart_educiaa_printf("Protocol: %cCOMMAND:PARAMS%c\r\n",
                        UART_FRAME_START, UART_FRAME_END);
    uart_educiaa_send_line("Commands: MV (move), ST (stop), GT (get telemetry)");
    uart_educiaa_send_line("Commands: SF (forward), SB (backward), SL (left), SR (right)");

    /* Loop principal */
    while (1)
    {
        /* Procesar datos UART entrantes */
        int new_commands = uart_educiaa_process(&g_uart_ctx);

        /* Procesar comandos encolados */
        if (new_commands > 0 || uart_educiaa_has_commands(&g_uart_ctx))
        {
            data_cmd_t cmd;
            while (uart_educiaa_get_command(&g_uart_ctx, &cmd) == 0)
            {
                process_rover_command(&cmd);
            }
        }

        /* Envío periódico de telemetría */
        g_telemetry_counter++;
        if (g_telemetry_counter >= TELEMETRY_PERIOD)
        {
            g_telemetry_counter = 0;
            g_telemetry.timestamp++;

            /* Simular variación en voltaje de batería */
            g_telemetry.battery_voltage = 12.5 + (g_telemetry.timestamp % 10) * 0.1;

            uart_educiaa_send_telemetry(&g_uart_ctx, &g_telemetry);

            /* Mostrar estadísticas cada cierto tiempo */
            uint32_t rx, tx, errors, overflows;
            uart_educiaa_get_stats(&g_uart_ctx, &rx, &tx, &errors, &overflows);
            uart_educiaa_printf("Stats: RX=%lu TX=%lu Errors=%lu Overflows=%lu\r\n",
                                rx, tx, errors, overflows);
        }

        /* Aquí puedes agregar otras tareas del sistema:
         * - Control de motores
         * - Lectura de sensores
         * - Control de LEDs
         * - etc.
         */
    }

    return 0;
}