/*
 * RS485.c
 *
 *  Created on: Sep 8, 2025
 *      Author: Shashank
 */

#include "ModBusRS485.h"
#include "Parameters.h"

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart4;

/* CRC-16 (Modbus) calculation */
static uint16_t crc16_modbus(const uint8_t *data, unsigned int length)
{
    uint16_t crc = 0xFFFF;
    for (unsigned int i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (unsigned int j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/*
 * Set RS485 DE (driver enable) pin: assumed active HIGH for TX, LOW for RX.
 * Adjust behavior if your transceiver uses separate DE/RE pins.
 */
static inline void rs485_set_tx_mode(void)
{
    HAL_GPIO_WritePin(RS485_DE_GPIO_Port, RS485_DE_Pin, GPIO_PIN_SET);
}

static inline void rs485_set_rx_mode(void)
{
    HAL_GPIO_WritePin(RS485_DE_GPIO_Port, RS485_DE_Pin, GPIO_PIN_RESET);
}

/*
 * rs485_send_command:
 * - Blocks to transmit the command using HAL_UART_Transmit
 * - Switches to RX and collects incoming bytes using HAL_UART_Receive in byte mode
 * - Dynamically adjusts expected response length when 3 bytes received (function code + byte count)
 *
 * Returns:
 *  >0 : number of bytes received
 *  -1 : timeout
 *  -2 : unknown response type
 *  -3 : rx buffer too small
 */
int rs485_send_command(UART_HandleTypeDef *huart,
                       const uint8_t *command, size_t cmd_len,
                       uint8_t *response, size_t resp_buf_len,
                       uint32_t timeout_ms)
{
    if (huart == NULL || command == NULL || response == NULL) {
        return -3;
    }

    // Debug print (optional)
    // printf("Command: ");
    // for (size_t i = 0; i < cmd_len; i++) printf("%02X ", command[i]);
    // printf("\n");

    // Enable TX
    rs485_set_tx_mode();
    HAL_Delay(2); // allow transceiver to settle (adjust if needed)

    // Transmit command (blocking)
    if (HAL_UART_Transmit(huart, (uint8_t *)command, (uint16_t)cmd_len, 1000) != HAL_OK) {
        rs485_set_rx_mode();
        return -1; // transmit error treated as timeout/error
    }

    // Optional: some devices expect a trailing empty byte or small delay
    // HAL_UART_Transmit(huart, (uint8_t[]){0x00}, 1, 100);

    // Switch to RX
    rs485_set_rx_mode();

    size_t expected_len = resp_buf_len; // may be adjusted after header
    size_t received = 0;
    uint32_t start_tick = HAL_GetTick();

    while (received < expected_len) {
        uint8_t byte;
        // Use a small per-byte timeout so we can re-check overall timeout.
        uint32_t per_byte_timeout = 20U; // ms, adjust as needed

        if (HAL_UART_Receive(huart, &byte, 1, per_byte_timeout) == HAL_OK) {
            if (received < resp_buf_len) {
                response[received++] = byte;
            } else {
                // Overflow (shouldn't happen if resp_buf_len correct)
                return -3;
            }

            // After receiving 3 bytes we can determine expected length for many Modbus frames
            if (received == 3) {
                uint8_t func = response[1];
                if (func == 0x03) {
                    // Read Holding Registers: response[2] == byte count
                    expected_len = 3 + response[2] + 2; // header(3) + data + crc(2)
                    if (expected_len > resp_buf_len) {
                        // user buffer too small
                        return -3;
                    }
                } else if (func == 0x06) {
                    // Write single register: echo of request (8 bytes including CRC)
                    expected_len = cmd_len;
                    if (expected_len > resp_buf_len) expected_len = resp_buf_len;
                } else if ((func & 0x80) != 0) {
                    // Exception responses are 5 bytes
                    expected_len = 5;
                    if (expected_len > resp_buf_len) expected_len = resp_buf_len;
                } else {
                    // Unknown function code
                    return -2;
                }
            }
        } else {
            // no byte received within per_byte_timeout, check overall timeout
            if ((HAL_GetTick() - start_tick) > timeout_ms) {
                return -1; // timeout
            }
            // otherwise continue waiting
        }
    }

    return (int)received;
}

/*
 * High-level function: sends Modbus command, does retries and validates CRC.
 * Returns:
 *  MODBUS_OK (0) on success
 *  negative error codes on failure
 */
int8_t ModBus_Read_Register(UART_HandleTypeDef *huart,
                            const uint8_t *command, size_t cmd_len,
                            uint8_t *response, size_t resp_buf_len)
{
    int8_t last_error = MODBUS_ERR_TIMEOUT;

    for (size_t attempt = 0; attempt < ModBusRetry; attempt++) {
        if (resp_buf_len > 0 && response) {
            memset(response, 0, resp_buf_len);
        }

        int resp_len = rs485_send_command(huart, command, cmd_len, response, resp_buf_len, ModBusTimeOut);
        if (resp_len < 0) {
            last_error = (int8_t)resp_len;
            // small backoff between retries
            HAL_Delay(100);
            continue;
        }

        if ((size_t)resp_len < 5) {
            last_error = MODBUS_ERR_INCOMPLETE;
            HAL_Delay(100);
            continue;
        }

        // CRC: Modbus RTU uses CRC low byte first then high byte
        uint16_t received_crc = (uint16_t)response[resp_len - 2] | ((uint16_t)response[resp_len - 1] << 8);
        uint16_t calc_crc = crc16_modbus(response, resp_len - 2);
        if (received_crc != calc_crc) {
            last_error = MODBUS_ERR_CRC_MISMATCH;
            HAL_Delay(100);
            continue;
        }

        // Success
        return MODBUS_OK;
    }

    return last_error;
}

/*
 * Build 8 byte Modbus RTU command with CRC (low, high).
 * cmdArray must be at least 8 bytes.
 */
void ModBus_CMD8byte_construct(uint8_t *cmdArray,
                               uint8_t SENSOR_ADDRESS,
                               uint8_t FUNC_READ_REGISTER,
                               uint16_t register_address,
                               uint16_t reg_count)
{
    if (cmdArray == NULL) return;

    // Layout:
    // [0] Slave Addr
    // [1] Func code
    // [2] Start Reg Hi
    // [3] Start Reg Lo
    // [4] Reg Count Hi
    // [5] Reg Count Lo
    // [6] CRC Low
    // [7] CRC High

    cmdArray[0] = SENSOR_ADDRESS;
    cmdArray[1] = FUNC_READ_REGISTER;
    cmdArray[2] = (uint8_t)((register_address >> 8) & 0xFF);
    cmdArray[3] = (uint8_t)(register_address & 0xFF);
    cmdArray[4] = (uint8_t)((reg_count >> 8) & 0xFF);
    cmdArray[5] = (uint8_t)(reg_count & 0xFF);

    uint16_t crc = crc16_modbus(cmdArray, 6);
    cmdArray[6] = (uint8_t)(crc & 0xFF);
    cmdArray[7] = (uint8_t)((crc >> 8) & 0xFF);
}



