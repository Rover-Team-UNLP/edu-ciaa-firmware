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
// int main(void)
// {
//    /* Inicializaciones */
//    Board_Init();
//    SystemCoreClockUpdate();
//    SysTick_Config(SystemCoreClock / 1000); /* Interrupción cada 1 ms */

//    // Inicializa la comunicación UART a 115200 baudios (igual que el ESP32)
//    uart_init(UART_BAUD_RATE);
//    uart_send_string_blocking("EDU-CIAA Rover Controller Inicializado.\n");

//    /* Variable para almacenar el comando recibido */
//    RoverCommand current_command;

//    /* Bucle principal */
//    while (1)
//    {
//       /* Verifica si hay un nuevo comando disponible desde UART */
//       if (uart_is_new_command_available())
//       {
//          uart_get_received_command(&current_command); // Obtiene el comando
//          // process_rover_command(current_command);      // Procesa el comando
//       }

//       /* Parpadeo de LED RGB azul como señal de vida (heartbeat) */
//       if (tick_counter >= TICK_DELAY_MS)
//       {
//          tick_counter = 0;
//          Board_LED_Toggle(LED_3); // LED Azul
//       }

//       /* Sleep hasta la próxima interrupción (SysTick o UART) */
//       __WFI();
//    }
//    return 0;
// }

int main()
{
   /* Inicializaciones */
   Board_Init();
   SystemCoreClockUpdate();
   SysTick_Config(SystemCoreClock / 1000); /* Interrupción cada 1 ms */

   // uart_init(UART_BAUD_RATE);
   /* Configura los pines de UART2 (P7_1=TX, P7_2=RX) */
   Chip_SCU_PinMuxSet(7, 1, (SCU_MODE_PULLDOWN | SCU_MODE_FUNC6));                      // UART2_TXD
   Chip_SCU_PinMuxSet(7, 2, (SCU_MODE_PULLDOWN | SCU_MODE_FUNC6 | SCU_MODE_INBUFF_EN)); // UART2_RXD

   /* Inicializa UART2 */
   Chip_UART_Init(LPC_USART2);
   Chip_UART_SetBaud(LPC_USART2, 115200);
   Chip_UART_ConfigData(LPC_USART2, UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS);
   Chip_UART_TXEnable(LPC_USART2);
   Chip_UART_SetupFIFOS(LPC_USART2, UART_FCR_FIFO_EN | UART_FCR_RX_RS | UART_FCR_TX_RS);

   const char *msg = "EDU-CIAA Echo Ready!\r\n";
   while (*msg)
   {
      while (!(Chip_UART_ReadLineStatus(LPC_USART2) & UART_LSR_THRE))
         ;
      Chip_UART_SendByte(LPC_USART2, *msg++);
   }

   while (1)
   {
      if (Chip_UART_ReadLineStatus(LPC_USART2) & UART_LSR_RDR)
      {
         /* Lee el byte recibido */
         uint8_t received_byte = Chip_UART_ReadByte(LPC_USART2);

         while (!(Chip_UART_ReadLineStatus(LPC_USART2) & UART_LSR_THRE))
            ;

         /* Hace echo del byte recibido */
         Chip_UART_SendByte(LPC_USART2, received_byte);

         /* Toggle LED para indicar actividad */
         Board_LED_Toggle(LED_3);
      }
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