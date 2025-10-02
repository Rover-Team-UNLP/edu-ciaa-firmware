#include "board.h"
#include "chip.h"
#include "sct_18xx_43xx.h"

#define TICKRATE_HZ (1000)
#define PWM_FREQ_HZ (1000)

typedef struct
{
   uint8_t red;
   uint8_t green;
   uint8_t blue;
} rgb_color_t;

static volatile uint32_t tick_ct = 0;

// Initialize PWM for RGB LED
void RGB_PWM_Init(void)
{
   // Enable SCT clock
   Chip_SCT_Init(LPC_SCT);

   // Configure SCT for PWM mode
   Chip_SCT_Config(LPC_SCT, SCT_CONFIG_32BIT_COUNTER | SCT_CONFIG_AUTOLIMIT_L);

   // Set PWM frequency (1kHz)
   uint32_t pwm_period = SystemCoreClock / PWM_FREQ_HZ;

   // Usar funciones de la API en lugar de acceso directo
   Chip_SCT_SetMatchCount(LPC_SCT, SCT_MATCH_0, pwm_period);
   Chip_SCT_SetMatchReload(LPC_SCT, SCT_MATCH_0, pwm_period);

   // Configure GPIO pins for PWM output
   Chip_SCU_PinMuxSet(0x1, 0, (SCU_MODE_INACT | SCU_MODE_FUNC1)); // GPIO2 -> SCT_OUT0
   Chip_SCU_PinMuxSet(0x1, 1, (SCU_MODE_INACT | SCU_MODE_FUNC1)); // GPIO4 -> SCT_OUT1
   Chip_SCU_PinMuxSet(0x1, 2, (SCU_MODE_INACT | SCU_MODE_FUNC1)); // GPIO6 -> SCT_OUT2

   // Configure PWM outputs usando la API
   Chip_SCT_SetMatchCount(LPC_SCT, SCT_MATCH_1, 0); // Red duty cycle
   Chip_SCT_SetMatchCount(LPC_SCT, SCT_MATCH_2, 0); // Green duty cycle
   Chip_SCT_SetMatchCount(LPC_SCT, SCT_MATCH_3, 0); // Blue duty cycle

   Chip_SCT_SetMatchReload(LPC_SCT, SCT_MATCH_1, 0);
   Chip_SCT_SetMatchReload(LPC_SCT, SCT_MATCH_2, 0);
   Chip_SCT_SetMatchReload(LPC_SCT, SCT_MATCH_3, 0);

   // Configure events using API functions
   Chip_SCT_EventSetControl(LPC_SCT, SCT_EVT_0,
                            SCT_EV_CTRL_MATCHSEL(SCT_MATCH_0) | SCT_EV_CTRL_COMBMODE(0));

   Chip_SCT_EventSetControl(LPC_SCT, SCT_EVT_1,
                            SCT_EV_CTRL_MATCHSEL(SCT_MATCH_1) | SCT_EV_CTRL_COMBMODE(0));

   Chip_SCT_EventSetControl(LPC_SCT, SCT_EVT_2,
                            SCT_EV_CTRL_MATCHSEL(SCT_MATCH_2) | SCT_EV_CTRL_COMBMODE(0));

   Chip_SCT_EventSetControl(LPC_SCT, SCT_EVT_3,
                            SCT_EV_CTRL_MATCHSEL(SCT_MATCH_3) | SCT_EV_CTRL_COMBMODE(0));

   // Configure outputs using API
   Chip_SCT_OutputSetAction(LPC_SCT, 0, SCT_EVT_0, 1); // Output 0, Event 0, SET
   Chip_SCT_OutputSetAction(LPC_SCT, 0, SCT_EVT_1, 2); // Output 0, Event 1, CLEAR

   Chip_SCT_OutputSetAction(LPC_SCT, 1, SCT_EVT_0, 1); // Output 1, Event 0, SET
   Chip_SCT_OutputSetAction(LPC_SCT, 1, SCT_EVT_2, 2); // Output 1, Event 2, CLEAR

   Chip_SCT_OutputSetAction(LPC_SCT, 2, SCT_EVT_0, 1); // Output 2, Event 0, SET
   Chip_SCT_OutputSetAction(LPC_SCT, 2, SCT_EVT_3, 2); // Output 2, Event 3, CLEAR

   // Start the SCT
   Chip_SCT_ClearControl(LPC_SCT, SCT_CTRL_HALT_L);
}

// Set RGB color (0-255 for each channel)
void RGB_SetColor(uint8_t red, uint8_t green, uint8_t blue)
{
   uint32_t pwm_period = SystemCoreClock / PWM_FREQ_HZ;

   // Convert 0-255 values to PWM duty cycle usando la API
   uint32_t red_duty = (red * pwm_period) / 255;
   uint32_t green_duty = (green * pwm_period) / 255;
   uint32_t blue_duty = (blue * pwm_period) / 255;

   Chip_SCT_SetMatchReload(LPC_SCT, SCT_MATCH_1, red_duty);   // Red
   Chip_SCT_SetMatchReload(LPC_SCT, SCT_MATCH_2, green_duty); // Green
   Chip_SCT_SetMatchReload(LPC_SCT, SCT_MATCH_3, blue_duty);  // Blue
}

void SysTick_Handler(void)
{
   tick_ct++;
}

void delay(uint32_t tk)
{
   uint32_t end = tick_ct + tk;
   while (tick_ct < end)
      __WFI();
}

int main(void)
{
   SystemCoreClockUpdate();
   Board_Init();
   SysTick_Config(SystemCoreClock / TICKRATE_HZ);
   RGB_PWM_Init();

   printf("RGB LED PWM Control Started\r\n");

   while (1)
   {
      // Test RGB colors
      printf("Red\r\n");
      RGB_SetColor(255, 0, 0);
      delay(1000);

      printf("Green\r\n");
      RGB_SetColor(0, 255, 0);
      delay(1000);

      printf("Blue\r\n");
      RGB_SetColor(0, 0, 255);
      delay(1000);

      printf("White\r\n");
      RGB_SetColor(255, 255, 255);
      delay(1000);

      printf("Off\r\n");
      RGB_SetColor(0, 0, 0);
      delay(1000);

      printf("Cycle at %d\r\n", tick_ct);
   }
}
