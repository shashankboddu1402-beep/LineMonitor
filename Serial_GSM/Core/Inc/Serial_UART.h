#ifndef SERIAL_UART_H
#define SERIAL_UART_H

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Adjust as you like
#define Serial_RXBufLen   2048

void Serial_Init(UART_HandleTypeDef *huart);

uint16_t Serial_Available(void);
uint8_t  Serial_ReadByte(uint8_t *byte);
uint8_t  Serial_BufferIsFull(void);
void     Serial_ClearOverflowFlag(void);

HAL_StatusTypeDef Serial_Write(const uint8_t *data, uint16_t len);
HAL_StatusTypeDef Serial_Print(const char *s);
HAL_StatusTypeDef Serial_Println(const char *s);

#ifdef __cplusplus
}
#endif

#endif // SERIAL_UART_H
