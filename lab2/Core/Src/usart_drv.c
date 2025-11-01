#include "usart_drv.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdbool.h>

#define UART_TX_BUF_SIZE 256

static UART_HandleTypeDef *huart_ptr = NULL;

/* RX */
static volatile uint8_t rx_ring[UART_RX_BUF_SIZE];
static volatile uint16_t rx_head = 0, rx_tail = 0;

/* TX */
static volatile uint8_t tx_ring[UART_TX_BUF_SIZE];
static volatile uint16_t tx_head = 0, tx_tail = 0;
static volatile bool tx_busy = false;

static volatile bool interrupts_enabled = true;
static bool echo = true;

void USART_DRV_Init(UART_HandleTypeDef *huart)
{
    huart_ptr = huart;
    rx_head = rx_tail = 0;
    tx_head = tx_tail = 0;
    tx_busy = false;
    interrupts_enabled = true;
    __HAL_UART_ENABLE_IT(huart_ptr, UART_IT_RXNE);
}

void USART_DRV_EnableInterrupts(bool en)
{
    if(!huart_ptr) return;
    interrupts_enabled = en;
    if(en) {
        __HAL_UART_ENABLE_IT(huart_ptr, UART_IT_RXNE);
        if(tx_head != tx_tail)
            __HAL_UART_ENABLE_IT(huart_ptr, UART_IT_TXE);
    } else {
        __HAL_UART_DISABLE_IT(huart_ptr, UART_IT_RXNE);
        __HAL_UART_DISABLE_IT(huart_ptr, UART_IT_TXE);
    }
}

bool USART_DRV_IsInterruptsEnabled(void){ return interrupts_enabled; }

bool USART_DRV_TxBuf(const uint8_t *buf, uint16_t len)
{
    if(!huart_ptr) return false;

    for(uint16_t i = 0; i < len; i++) {
        uint16_t next = (tx_head + 1) % UART_TX_BUF_SIZE;
        if(next == tx_tail) {
            // буфер переполнен
            return false;
        }
        tx_ring[tx_head] = buf[i];
        tx_head = next;
    }

    if(interrupts_enabled) {
        if(!tx_busy) {
            tx_busy = true;
            __HAL_UART_ENABLE_IT(huart_ptr, UART_IT_TXE);
        }
    }

    return true;
}

void USART_DRV_PollTx(void)
{
    if(interrupts_enabled || !huart_ptr) return;

    if((tx_head != tx_tail) && (__HAL_UART_GET_FLAG(huart_ptr, UART_FLAG_TXE))) {
        huart_ptr->Instance->DR = tx_ring[tx_tail];
        tx_tail = (tx_tail + 1) % UART_TX_BUF_SIZE;
    }
}

bool USART_DRV_TxStr(const char *s)
{
    return USART_DRV_TxBuf((uint8_t*)s, (uint16_t)strlen(s));
}

bool USART_DRV_PollGetByte(uint8_t *out)
{
    if(USART_DRV_IsInterruptsEnabled()) {
        if(rx_head == rx_tail) return false;

        *out = rx_ring[rx_tail++];
        if(rx_tail >= UART_RX_BUF_SIZE) rx_tail = 0;

        if(echo) {
            uint8_t ch = *out;
            USART_DRV_TxBuf(&ch, 1);
        }

        return true;
    } else {
        if(__HAL_UART_GET_FLAG(huart_ptr, UART_FLAG_RXNE)) {
            *out = (uint8_t)(huart_ptr->Instance->DR & 0xFF);
            if(echo) {
                uint8_t ch = *out;
                USART_DRV_TxBuf(&ch, 1);
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

    /* --- RX --- */
    if(sr & USART_SR_RXNE) {
        uint8_t b = (uint8_t)(huart_ptr->Instance->DR & 0xFF);
        uint16_t next = (rx_head + 1) % UART_RX_BUF_SIZE;
        if(next == rx_tail) {
            // переполнение — дропаем старый байт
            rx_tail = (rx_tail + 1) % UART_RX_BUF_SIZE;
        }
        rx_ring[rx_head] = b;
        rx_head = next;
    }

    /* --- TX --- */
    if((sr & USART_SR_TXE) && tx_head != tx_tail) {
        huart_ptr->Instance->DR = tx_ring[tx_tail];
        tx_tail = (tx_tail + 1) % UART_TX_BUF_SIZE;
    }

    if(tx_head == tx_tail) {
        __HAL_UART_DISABLE_IT(huart_ptr, UART_IT_TXE);
        tx_busy = false;
    }
}
