#include "usart_drv.h"
#include "stm32f4xx_hal.h"
#include <string.h>

static UART_HandleTypeDef *huart_ptr = NULL;
static volatile uint8_t rx_ring[UART_RX_BUF_SIZE];
static volatile uint16_t rx_head = 0, rx_tail = 0;
static volatile bool interrupts_enabled = true;
static bool echo = true;

void USART_DRV_Init(UART_HandleTypeDef *huart)
{
    huart_ptr = huart;
    rx_head = rx_tail = 0;
    interrupts_enabled = true;
    __HAL_UART_ENABLE_IT(huart_ptr, UART_IT_RXNE);
}

void USART_DRV_EnableInterrupts(bool en)
{
    if(!huart_ptr) return;
    interrupts_enabled = en;
    if(en) {
        __HAL_UART_ENABLE_IT(huart_ptr, UART_IT_RXNE);
    } else {
        __HAL_UART_DISABLE_IT(huart_ptr, UART_IT_RXNE);
    }
}

bool USART_DRV_IsInterruptsEnabled(void){ return interrupts_enabled; }

bool USART_DRV_TxStr(const char *s)
{
    if(!huart_ptr) return false;
    uint16_t len = (uint16_t)strlen(s);
    if(HAL_UART_Transmit(huart_ptr, (uint8_t*)s, len, 100) == HAL_OK) return true;
    return false;
}
bool USART_DRV_TxBuf(const uint8_t *buf, uint16_t len)
{
    if(!huart_ptr) return false;
    if(HAL_UART_Transmit(huart_ptr, (uint8_t*)buf, len, 200) == HAL_OK) return true;
    return false;
}

bool USART_DRV_PollGetByte(uint8_t *out)
{
    if(USART_DRV_IsInterruptsEnabled()) {
        if(rx_head == rx_tail) return false;

        *out = rx_ring[rx_tail++];
        if(rx_tail >= UART_RX_BUF_SIZE) rx_tail = 0;

        if(echo) {
            uint8_t ch = *out;
            HAL_UART_Transmit(huart_ptr, &ch, 1, 20);
        }

        return true;
    } else {
        if(__HAL_UART_GET_FLAG(huart_ptr, UART_FLAG_RXNE)) {
            *out = (uint8_t)(huart_ptr->Instance->DR & 0xFF);
            if(echo) {
                uint8_t ch = *out;
                HAL_UART_Transmit(huart_ptr, &ch, 1, 20);
            }
            return true;
        }
        return false;
    }
}


void USART_DRV_SetEcho(bool en){ echo = en; }

void USART_DRV_IRQ_Handler(void)
{
    if(!huart_ptr) return;
    uint32_t sr = huart_ptr->Instance->SR;
    if(sr & USART_SR_RXNE) {
        uint8_t b = (uint8_t)(huart_ptr->Instance->DR & 0xFF);

        uint16_t next = (rx_head + 1);
        if(next >= UART_RX_BUF_SIZE) next = 0;

        if(next == rx_tail) {
            rx_tail++; if(rx_tail >= UART_RX_BUF_SIZE) rx_tail = 0;
        }

        rx_ring[rx_head] = b;
        rx_head = next;
    }
}

