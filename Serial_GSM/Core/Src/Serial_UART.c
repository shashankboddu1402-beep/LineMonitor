/*
 * Serial_UART.c  (DMA-based RX)
 */

#include "Serial_UART.h"
#include <string.h>

static UART_HandleTypeDef *Serial_huart = NULL;

// DMA RX buffer (hardware writes here in circular mode)
static uint8_t Serial_rx_buffer[Serial_RXBufLen];

// Our read index into the circular buffer
static volatile uint16_t Serial_rx_tail = 0;

// Overflow flag (DMA overwrote unread data)
static volatile uint8_t Serial_overflow = 0;

// Get current DMA write position (index 0..Serial_RXBufLen-1)
static uint16_t Serial_GetDmaWriteIndex(void)
{
    if (Serial_huart == NULL || Serial_huart->hdmarx == NULL)
        return 0;

    // NDTR = how many bytes *remain* to be transferred
    uint16_t remaining = __HAL_DMA_GET_COUNTER(Serial_huart->hdmarx);

    // So write index = total - remaining
    return (uint16_t)(Serial_RXBufLen - remaining);
}

// ---------- Public functions ----------

void Serial_Init(UART_HandleTypeDef *huart)
{
    Serial_huart = huart;

    Serial_rx_tail    = 0;
    Serial_overflow   = 0;

    // Start DMA RX in circular mode into Serial_rx_buffer
    // Make sure in CubeMX: UARTx RX DMA set to Circular
    HAL_UART_Receive_DMA(Serial_huart, Serial_rx_buffer, Serial_RXBufLen);
}

uint16_t Serial_Available(void)
{
    uint16_t write = Serial_GetDmaWriteIndex();
    uint16_t tail  = Serial_rx_tail;

    uint16_t available;

    if (write >= tail)
    {
        available = (uint16_t)(write - tail);
    }
    else
    {
        available = (uint16_t)(Serial_RXBufLen - tail + write);
    }

    // Simple overflow detection:
    // if buffer is completely full, we lost some data
    if (available >= (Serial_RXBufLen - 1))
    {
        Serial_overflow = 1;
    }

    return available;
}

uint8_t Serial_ReadByte(uint8_t *byte)
{
    if (byte == NULL)
        return 0;

    if (Serial_Available() == 0)
        return 0;

    *byte = Serial_rx_buffer[Serial_rx_tail];
    Serial_rx_tail = (uint16_t)((Serial_rx_tail + 1) % Serial_RXBufLen);
    return 1;
}

uint8_t Serial_BufferIsFull(void)
{
    return Serial_overflow;
}

void Serial_ClearOverflowFlag(void)
{
    Serial_overflow = 0;
}

HAL_StatusTypeDef Serial_Write(const uint8_t *data, uint16_t len)
{
    if (Serial_huart == NULL || data == NULL || len == 0)
        return HAL_ERROR;

    return HAL_UART_Transmit(Serial_huart, (uint8_t *)data, len, HAL_MAX_DELAY);
}

HAL_StatusTypeDef Serial_Print(const char *s)
{
    if (s == NULL)
        return HAL_ERROR;

    return Serial_Write((const uint8_t *)s, (uint16_t)strlen(s));
}

HAL_StatusTypeDef Serial_Println(const char *s)
{
    HAL_StatusTypeDef status;

    if (s != NULL)
    {
        status = Serial_Print(s);
        if (status != HAL_OK)
            return status;
    }

    return Serial_Print("\r\n");
}
