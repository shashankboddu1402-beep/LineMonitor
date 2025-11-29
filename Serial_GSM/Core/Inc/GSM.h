/*
 * GSM.h
 *
 *  Created on: Nov 18, 2025
 *      Author: fervi
 */

#ifndef INC_GSM_H_
#define INC_GSM_H_


#define UartBufLen  2300
#define payBufLen  1400

typedef enum {
  InReset,
  GSMPowerOff,
  GSMUARTStabile,
  NotInReset
} GSMResetStatus;
//TODO movie to rtc handle files
typedef struct {
  uint8_t Hour;
  uint8_t Second;
  uint8_t Minute;
  uint8_t Year;
  uint8_t Month;
  uint8_t Date;
  uint8_t PreDate;
  char DataStamp[13];
  char TimeStamp[26];
} TimeStampLog;

typedef struct
{
    uint8_t  FaildCount;
    uint8_t  ResetStatus;
    int16_t RSSI;
    uint8_t MessStarus;
    uint8_t ResetMCU;
} GSMPar;
typedef struct {
  char ConfigURLHold[250];
  uint16_t ConfigfileSize;
  uint8_t OTARequest;
} OTAPra;



uint8_t atSend(const char *AT, const char *respons, uint32_t responseTime, uint16_t tryFor);
void GSMInit(void);
uint8_t GSMConnectMqtt(void);
void handleReceivedData(const char *data, uint8_t FromSMS);
void GSM_ProcessStoredSmsFromMaster_Reassemble(void);

#endif /* INC_GSM_H_ */
