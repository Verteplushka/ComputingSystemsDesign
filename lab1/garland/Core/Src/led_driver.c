#include "led_driver.h"
#include "stm32f4xx_hal.h"

static uint16_t cur_mask = 0;

void LED_Init(void)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LED_GREEN_PIN | LED_RED_PIN | LED_YELLOW_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    // start with all off
    HAL_GPIO_WritePin(LED_PORT, LED_GREEN_PIN|LED_RED_PIN|LED_YELLOW_PIN, GPIO_PIN_RESET);
    cur_mask = 0;
}

void LED_SetMask(uint16_t mask)
{
    // mask bits: 0->green,1->red,2->yellow
    HAL_GPIO_WritePin(LED_PORT, LED_GREEN_PIN, (mask & 0x1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED_RED_PIN,   (mask & 0x2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED_YELLOW_PIN,(mask & 0x4) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    cur_mask = mask & 0x7;
}

void LED_ToggleMask(uint16_t mask)
{
    if(mask & 0x1) HAL_GPIO_TogglePin(LED_PORT, LED_GREEN_PIN);
    if(mask & 0x2) HAL_GPIO_TogglePin(LED_PORT, LED_RED_PIN);
    if(mask & 0x4) HAL_GPIO_TogglePin(LED_PORT, LED_YELLOW_PIN);

    // recalc cur_mask (read pins)
    cur_mask = 0;
    if(HAL_GPIO_ReadPin(LED_PORT, LED_GREEN_PIN) == GPIO_PIN_SET) cur_mask |= 0x1;
    if(HAL_GPIO_ReadPin(LED_PORT, LED_RED_PIN)   == GPIO_PIN_SET) cur_mask |= 0x2;
    if(HAL_GPIO_ReadPin(LED_PORT, LED_YELLOW_PIN)== GPIO_PIN_SET) cur_mask |= 0x4;
}

uint16_t LED_GetMask(void)
{
    return cur_mask;
}
