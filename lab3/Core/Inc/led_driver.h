#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <stdint.h>

#define LED_GREEN_PIN    GPIO_PIN_13
#define LED_RED_PIN      GPIO_PIN_14
#define LED_YELLOW_PIN   GPIO_PIN_15
#define LED_PORT         GPIOD

void LED_Init(void);
void LED_SetMask(uint16_t mask); // бит 0 -> green, 1 -> red, 2 -> yellow
void LED_ToggleMask(uint16_t mask);
uint16_t LED_GetMask(void);

#endif
