#include "board.h"
#include "communication.h"
#include "uart_comm.h"
#include <string.h>

#define TICK_DELAY_MS 500

static uint32_t tick_counter = 0; // Contador para el delay del LED

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
         // process_rover_command(current_command);      // Procesa el comando
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