/*
 * RS485.h
 *
 *  Created on: Sep 8, 2025
 *      Author: Shashank
 */

#ifndef INC_MODBUSRS485_H_
#define INC_MODBUSRS485_H_


#include "stdint.h"
#include "stddef.h"

// Configure these in your main.h or change here:
#include "main.h" // for UART handle and GPIO pins

// Example expectations from main.h (change names to match your project):
// extern UART_HandleTypeDef huart1;
// #define RS485_DE_GPIO_Port GPIOA
// #define RS485_DE_Pin       GPIO_PIN_2

#define ModBusRetry    5
#define ModBusTimeOut  2000U // ms

// Return codes for Modbus read wrapper
#define MODBUS_OK                        0
#define MODBUS_ERR_TIMEOUT              -1
#define MODBUS_ERR_INCOMPLETE           -2
#define MODBUS_ERR_CRC_MISMATCH         -3
#define MODBUS_ERR_UNKNOWN_RESPONSE     -4

// Build 8 byte Modbus RTU command (addr, func, start reg, reg count) with CRC
void ModBus_CMD8byte_construct(uint8_t *cmdArray,
                               uint8_t SENSOR_ADDRESS,
                               uint8_t FUNC_READ_REGISTER,
                               uint16_t register_address,
                               uint16_t reg_count);

// Send raw command over RS485 (HAL UART) and receive response
// Returns number of bytes received (>0) or negative error
int rs485_send_command(UART_HandleTypeDef *huart,
                       const uint8_t *command, size_t cmd_len,
                       uint8_t *response, size_t resp_buf_len,
                       uint32_t timeout_ms);

// High-level: send command, validate CRC and return MODBUS_OK or error code
int8_t ModBus_Read_Register(UART_HandleTypeDef *huart,
                            const uint8_t *command, size_t cmd_len,
                            uint8_t *response, size_t resp_buf_len);


#endif /* INC_MODBUSRS485_H_ */
