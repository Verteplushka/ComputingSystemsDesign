#include "button_driver.h"
#include "stm32f4xx_hal.h"

#define BUTTON_GPIO_PORT GPIOC
#define BUTTON_GPIO_PIN  GPIO_PIN_15
#define DEBOUNCE_DELAY_MS 50  // 50 мс задержка для антидребезга

static bool last_state = false;
static bool was_pressed = false;
static uint32_t last_tick = 0;

void BUTTON_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = BUTTON_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;  // подтяжка вверх
    HAL_GPIO_Init(BUTTON_GPIO_PORT, &GPIO_InitStruct);
}

void BUTTON_Process(void) {
    bool pressed = (HAL_GPIO_ReadPin(BUTTON_GPIO_PORT, BUTTON_GPIO_PIN) == GPIO_PIN_RESET);

    uint32_t tick = HAL_GetTick();
    if (pressed != last_state) {
        // Если прошло достаточно времени с последнего изменения
        if ((tick - last_tick) >= DEBOUNCE_DELAY_MS) {
            last_state = pressed;
            last_tick = tick;
            if (pressed) {
                was_pressed = true;
            }
        }
    } else {
        last_tick = tick; // сброс таймера, если состояние стабильно
    }
}

bool BUTTON_WasPressed(void) {
    if (was_pressed) {
        was_pressed = false;
        return true;
    }
    return false;
}
