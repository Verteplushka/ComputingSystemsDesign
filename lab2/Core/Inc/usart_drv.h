#ifndef USART_DRV_H
#define USART_DRV_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define UART_RX_BUF_SIZE 128

void USART_DRV_Init(UART_HandleTypeDef *huart);
void USART_DRV_EnableInterrupts(bool en);
bool USART_DRV_IsInterruptsEnabled(void);
bool USART_DRV_TxStr(const char *s);
bool USART_DRV_TxBuf(const uint8_t *buf, uint16_t len);

bool USART_DRV_PollGetByte(uint8_t *out);

void USART_DRV_SetEcho(bool en);

void USART_DRV_IRQ_Handler(void);

#endif
