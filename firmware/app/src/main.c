#include "board.h"
#include "communication.h"
#include "uart_comm.h"
#include <string.h>

#define TICK_DELAY_MS 500

/**
 * @brief Función para procesar el comando del rover recibido por UART.
 * @param cmd : Estructura con el comando parseado.
 */
static void process_rover_command(RoverCommand cmd);

/**
 * @brief Función placeholder para controlar los motores (a implementar).
 * @param speed_m1 : Velocidad para el motor 1 (-100 a 100).
 * @param speed_m2 : Velocidad para el motor 2 (-100 a 100).
 */
static void control_motors(int speed_m1, int speed_m2);

static uint32_t tick_counter = 0; // Contador para el delay del LED

/**
 * @brief Procesa comandos del rover con el struct RoverCommand.
 */
static void process_rover_command(RoverCommand cmd)
{
   if (!cmd.valid)
   {
      // Podríamos enviar un mensaje de error o simplemente ignorar
      uart_send_string_blocking("Error: Comando Invalido\n");
      return;
   }

   switch (cmd.cmd_type)
   {
   case CMD_MOVE_FORWARD:
      Board_LED_Set(LED_1, true);
      Board_LED_Set(LED_2, false);
      control_motors(cmd.speed_M1, cmd.speed_M2);
      uart_send_string_blocking("OK: FORWARD\n");
      break;

   case CMD_MOVE_BACKWARDS:
      Board_LED_Set(LED_1, false);
      Board_LED_Set(LED_2, true);
      control_motors(cmd.speed_M1, cmd.speed_M2);
      uart_send_string_blocking("OK: BACKWARDS\n");
      break;

   case CMD_MOVE_LEFT:
      Board_LED_Set(LED_1, true);
      Board_LED_Set(LED_2, true);
      control_motors(cmd.speed_M1, cmd.speed_M2);
      uart_send_string_blocking("OK: LEFT\n");
      break;

   case CMD_MOVE_RIGHT:
      Board_LED_Set(LED_1, true);
      Board_LED_Set(LED_2, true);
      control_motors(cmd.speed_M1, cmd.speed_M2);
      uart_send_string_blocking("OK: RIGHT\n");
      break;

   default:
      // Comando desconocido (no debería ocurrir si parse_command_string valida bien)
      uart_send_string_blocking("Error: Comando Desconocido\n");
      control_motors(0, 0); // Detener por seguridad
      break;
   }
}

/**
 * @brief Control de Motores
 *
 * Envia comandos de control a los motores. Por ahora es un placeholder para debugear.
 */
static void control_motors(int speed_m1, int speed_m2)
{
   // Placeholder: Imprime velocidades recibidas por UART (útil para debug inicial)
   char debug_msg[50];
   sprintf(debug_msg, "Motores: M1=%d, M2=%d\n", speed_m1, speed_m2);
   uart_send_string_blocking(debug_msg);
}

/**
 * @brief Main function
 *
 * This is the main entry point of the software.
 *
 * @returns 0 : Never returns.
 */
int main(void)
{
   /* Inicializaciones */
   Board_Init();
   SystemCoreClockUpdate();
   SysTick_Config(SystemCoreClock / 1000); /* Interrupción cada 1 ms */

   // Inicializa la comunicación UART a 115200 baudios (igual que el ESP32)
   uart_init(UART_BAUD_RATE);
   uart_send_string_blocking("EDU-CIAA Rover Controller Inicializado.\n");

   /* Variable para almacenar el comando recibido */
   RoverCommand current_command;

   /* Bucle principal */
   while (1)
   {
      /* Verifica si hay un nuevo comando disponible desde UART */
      if (uart_is_new_command_available())
      {
         uart_get_received_command(&current_command); // Obtiene el comando
         process_rover_command(current_command);      // Procesa el comando
      }

      /* Parpadeo de LED RGB azul como señal de vida (heartbeat) */
      if (tick_counter >= TICK_DELAY_MS)
      {
         tick_counter = 0;
         Board_LED_Toggle(LED_3); // LED Azul
      }

      /* Sleep hasta la próxima interrupción (SysTick o UART) */
      __WFI();
   }
   return 0;
}

/**
 * @brief Handler de la interrupción del SysTick cada 1ms.
 */
void SysTick_Handler(void)
{
   // Incrementa el contador para el delay del parpadeo del LED
   tick_counter++;
}