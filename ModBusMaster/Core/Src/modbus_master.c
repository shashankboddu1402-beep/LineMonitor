/*
 * modbus_master.c
 *
 *  Created on: Sep 17, 2025
 *      Author: Shashank
 */

#include "modbus_master.h"
extern UART_HandleTypeDef huart2;
// === RS485 Direction ===
static inline void RS485_SetTX(void) { HAL_GPIO_WritePin(RS485_DIR_PORT, RS485_DIR_PIN, GPIO_PIN_SET); }
static inline void RS485_SetRX(void) { HAL_GPIO_WritePin(RS485_DIR_PORT, RS485_DIR_PIN, GPIO_PIN_RESET); }

// === CRC16 Modbus ===
static uint16_t Modbus_CRC16(const uint8_t *buf, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x0001) { crc >>= 1; crc ^= 0xA001; }
            else { crc >>= 1; }
        }
    }
    return crc;
}

// returns: number of bytes written to buf (including CRC), or 0 on error (e.g. buffer too small or unsupported func)
uint16_t Modbus_BuildCmd(uint8_t *buf, uint16_t bufSize,
                        uint8_t slaveId, uint8_t func,
                        uint16_t startReg,
                        const uint16_t *values, uint16_t numRegs)
{
    // minimal checks
    if (!buf) return 0;

    switch (func) {
    case 0x03: // Read Holding Registers: [ID][0x03][addrHi][addrLo][qtyHi][qtyLo][crcLo][crcHi] -> 8 bytes
        if (bufSize < 8) return 0;
        buf[0] = slaveId;
        buf[1] = 0x03;
        buf[2] = (uint8_t)(startReg >> 8);
        buf[3] = (uint8_t)(startReg & 0xFF);
        buf[4] = (uint8_t)(numRegs >> 8);
        buf[5] = (uint8_t)(numRegs & 0xFF);
        {
            uint16_t crc = Modbus_CRC16(buf, 6);
            buf[6] = (uint8_t)(crc & 0xFF);
            buf[7] = (uint8_t)(crc >> 8);
        }
        return 8;

    case 0x06: // Write Single Register: [ID][0x06][addrHi][addrLo][valHi][valLo][crcLo][crcHi] -> 8 bytes
        if (bufSize < 8) return 0;
        buf[0] = slaveId;
        buf[1] = 0x06;
        buf[2] = (uint8_t)(startReg >> 8);
        buf[3] = (uint8_t)(startReg & 0xFF);
        // for write single the 'numRegs' is not used; use values[0] as the value
        if (!values) return 0;
        buf[4] = (uint8_t)(values[0] >> 8);
        buf[5] = (uint8_t)(values[0] & 0xFF);
        {
            uint16_t crc = Modbus_CRC16(buf, 6);
            buf[6] = (uint8_t)(crc & 0xFF);
            buf[7] = (uint8_t)(crc >> 8);
        }
        return 8;

    case 0x10: // Write Multiple Registers:
        // frame: [ID][0x10][addrHi][addrLo][qtyHi][qtyLo][byteCount][data...][crcLo][crcHi]
        {
            uint16_t byteCount = (uint16_t)(numRegs * 2);
            // total frame size = 7 (through byteCount) + byteCount + 2 (crc) = 9 + 2*numRegs
            uint32_t total = 9u + (uint32_t)2u * (uint32_t)numRegs;
            if (total > bufSize) return 0;
            if (!values && numRegs) return 0;
            buf[0] = slaveId;
            buf[1] = 0x10;
            buf[2] = (uint8_t)(startReg >> 8);
            buf[3] = (uint8_t)(startReg & 0xFF);
            buf[4] = (uint8_t)(numRegs >> 8);
            buf[5] = (uint8_t)(numRegs & 0xFF);
            buf[6] = (uint8_t)(byteCount & 0xFF);
            for (uint16_t i = 0; i < numRegs; i++) {
                buf[7 + 2*i]     = (uint8_t)(values[i] >> 8);
                buf[7 + 2*i + 1] = (uint8_t)(values[i] & 0xFF);
            }
            uint16_t crc = Modbus_CRC16(buf, (uint16_t)(7 + byteCount));
            buf[7 + byteCount]     = (uint8_t)(crc & 0xFF);
            buf[7 + byteCount + 1] = (uint8_t)(crc >> 8);
            return (uint16_t)total;
        }

    default:
        // unsupported function code
        return 0;
    }
}

// === Send + Receive ===
static ModbusStatus Modbus_SendReceive(uint8_t *cmd, uint16_t cmdLen,
                                       uint8_t *resp, uint16_t respBufSize, uint16_t *respLen)
{
    for (int attempt = 0; attempt < MB_RETRY; attempt++) {
        RS485_SetTX();
        HAL_Delay(1);
        if (HAL_UART_Transmit(&huart2, cmd, cmdLen, MB_TIMEOUT_MS) != HAL_OK) {
            RS485_SetRX();
            continue;
        }
        HAL_Delay(1);
        RS485_SetRX();

        uint32_t tickstart = HAL_GetTick();
        uint16_t received = 0;
        uint16_t expected = respBufSize; // start with max

        while ((HAL_GetTick() - tickstart) < MB_TIMEOUT_MS) {
            uint8_t byte;
            if (HAL_UART_Receive(&huart2, &byte, 1, 10) == HAL_OK) {
                resp[received++] = byte;

                if (received == 3) { // function code known
                    switch (resp[1]) {
                        case 0x03: expected = 3 + resp[2] + 2; break;
                        case 0x06: expected = cmdLen; break;
                        case 0x10: expected = 8; break;
                        case 0x83: case 0x86: expected = 5; break;
                        default: return MB_UNKNOWN;
                    }
                }
                if (received >= expected) {
                    *respLen = received;
                    // CRC check
                    uint16_t crcRx = (resp[received-1] << 8) | resp[received-2];
                    uint16_t crcCalc = Modbus_CRC16(resp, received-2);
                    return (crcRx == crcCalc) ? MB_OK : MB_CRC_ERR;
                }
            }
        }
    }
    return MB_TIMEOUT;
}

// === Public APIs ===
ModbusStatus Modbus_ReadHolding(uint8_t slaveId, uint16_t startReg, uint16_t numRegs,
                                uint16_t *regs, uint16_t maxRegs)
{
    uint8_t cmd[8], resp[256];
    uint16_t respLen = 0;

    cmd[0] = slaveId;
    cmd[1] = 0x03;
    cmd[2] = startReg >> 8; cmd[3] = startReg & 0xFF;
    cmd[4] = numRegs >> 8;  cmd[5] = numRegs & 0xFF;
    uint16_t crc = Modbus_CRC16(cmd, 6);
    cmd[6] = crc & 0xFF; cmd[7] = crc >> 8;

    ModbusStatus st = Modbus_SendReceive(cmd, 8, resp, sizeof(resp), &respLen);
    if (st != MB_OK) return st;

    if (resp[1] == 0x83) return MB_EXCEPTION;
    uint8_t byteCount = resp[2];
    if (byteCount/2 > maxRegs) return MB_INCOMPLETE;

    for (int i=0; i<numRegs; i++) {
        regs[i] = (resp[3+2*i] << 8) | resp[4+2*i];
    }
    return MB_OK;
}

ModbusStatus Modbus_WriteSingle(uint8_t slaveId, uint16_t regAddr, uint16_t value)
{
    uint8_t cmd[8], resp[16];
    uint16_t respLen = 0;

    cmd[0] = slaveId;
    cmd[1] = 0x06;
    cmd[2] = regAddr >> 8; cmd[3] = regAddr & 0xFF;
    cmd[4] = value >> 8;   cmd[5] = value & 0xFF;
    uint16_t crc = Modbus_CRC16(cmd, 6);
    cmd[6] = crc & 0xFF; cmd[7] = crc >> 8;

    return Modbus_SendReceive(cmd, 8, resp, sizeof(resp), &respLen);
}

ModbusStatus Modbus_WriteMultiple(uint8_t slaveId, uint16_t startReg,
                                  const uint16_t *values, uint16_t numRegs)
{
    uint8_t cmd[256], resp[16];
    uint16_t respLen = 0;

    cmd[0] = slaveId;
    cmd[1] = 0x10;
    cmd[2] = startReg >> 8; cmd[3] = startReg & 0xFF;
    cmd[4] = numRegs >> 8;  cmd[5] = numRegs & 0xFF;
    cmd[6] = numRegs*2; // byte count

    for (int i=0; i<numRegs; i++) {
        cmd[7+2*i] = values[i] >> 8;
        cmd[8+2*i] = values[i] & 0xFF;
    }
    uint16_t crc = Modbus_CRC16(cmd, 7+2*numRegs);
    cmd[7+2*numRegs] = crc & 0xFF;
    cmd[8+2*numRegs] = crc >> 8;

    return Modbus_SendReceive(cmd, 9+2*numRegs, resp, sizeof(resp), &respLen);
}

