/*
 * OTA_Uploade.h
 *
 *      Author: Shashank
 */

#ifndef INC_OTA_UPLOAD_H_
#define INC_OTA_UPLOAD_H_

#include <stdbool.h>
#include "main.h"
#include "OTA_Flash_Config.h"

#define OTA_Pra_SOF  0xAA    // Start of Frame
#define OTA_Pra_EOF  0xBB    // End of Frame
#define OTA_Pra_ACK  0x00    // ACK
#define OTA_Pra_NACK 0x01    // NACK
#define OTA_Pra_Ready 0x02    // Ready

#define OTA_Pra_DATA_MAX_SIZE ( 1024 )  //Maximum data Size
#define OTA_Pra_DATA_OVERHEAD (    9 )  //data overhead
#define OTA_Pra_PACKET_MAX_SIZE ( OTA_Pra_DATA_MAX_SIZE + OTA_Pra_DATA_OVERHEAD )


#define OTA_SLOT_MAX_SIZE        (106 * 1024)  //Each slot size (512KB)
/*
 * Reboot reason
 */
#define OTA_FIRST_TIME_BOOT       ( 0xFFFFFFFF )      //First time boot
#define OTA_NORMAL_BOOT           ( 0xBEEFFEED )      //Normal Boot
#define OTA_REQUEST           	  ( 0xDEADBEEF )      //OTA request by application
#define OTA_LOAD_PREV_APP         ( 0xFACEFADE )      //App requests to load the previous version
/*
 * Exception codes
 */
typedef enum
{
	OTA_Value_Fun_ERR      = 1,    // Failure
	OTA_Value_Fun_OK       = 2,    // Success

}OTA_Pra_FunStatus;

/*
 * OTA process state
 */
typedef enum
{
  OTA_Value_STATE_IDLE    = 0,
  OTA_Value_STATE_START   = 1,
  OTA_Value_STATE_HEADER  = 2,
  OTA_Value_STATE_DATA    = 3,
  OTA_Value_STATE_END     = 4,
}OTA_Pra_STATE_;

/*
 * Packet type
 */
typedef enum
{
  OTA_Value_PACKET_TYPE_CMD       = 0,    // Command
  OTA_Value_PACKET_TYPE_DATA      = 1,    // Data
  OTA_Value_PACKET_TYPE_HEADER    = 2,    // Header
  OTA_Value_PACKET_TYPE_RESPONSE  = 3,    // Response
}OTA_Pra_PACKET_TYPE_;

/*
 * OTA Commands
 */
typedef enum
{
	OTA_Value_CMD_START = 0,    // OTA Start command
	OTA_Value_CMD_END   = 1,    // OTA End command
	OTA_Value_CMD_ABORT = 2,    // OTA Abort command
}OTA_Pra_CMD_;

/*
 * Exception codes
 */

//Struct
/*
 * Slot table
 */
typedef struct
{
    uint8_t  is_this_slot_not_valid;  //Is this slot has a valid firmware/application?
    uint8_t  should_we_load_this_fw;   //Do we have to run this slot's firmware?
    uint32_t fw_size;                 //Slot's firmware/application size
    uint32_t fw_crc;                  //Slot's firmware/application CRC
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
}__attribute__((packed)) OTA_Pra_SLOT_;

/*
 * General configuration
 */
typedef struct
{
    uint32_t  reboot_cause;
    OTA_Pra_SLOT_ slot_table;
}__attribute__((packed)) OTA_Pra_GNRL_CFG_;


//Data frame
typedef struct
{
  uint32_t package_size;
  uint32_t package_crc;
  uint8_t  FlashSlot;
  uint32_t reserved1;
  uint32_t reserved2;
}__attribute__((packed)) meta_info;

/*
 * OTA Command format
 *
 * ________________________________________
 * |     | Packet |     |     |     |     |
 * | SOF | Type   | Len | CMD | CRC | EOF |
 * |_____|________|_____|_____|_____|_____|
 *   1B      1B     2B    1B     4B    1B
 */
typedef struct
{
  uint8_t   sof;
  uint8_t   packet_type;
  uint16_t  data_len;
  uint8_t   cmd;
  uint32_t  crc;
  uint8_t   eof;
}__attribute__((packed)) OTA_Pra_COMMAND_;

/*
 * OTA Header format
 *
 * __________________________________________
 * |     | Packet |     | Header |     |     |
 * | SOF | Type   | Len |  Data  | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B     16B     4B    1B
 */
typedef struct
{
  uint8_t     sof;
  uint8_t     packet_type;
  uint16_t    data_len;
  meta_info   meta_data;
  uint32_t    crc;
  uint8_t     eof;
}__attribute__((packed)) OTA_Pra_HEADER_;

/*
 * OTA Data format
 *
 * __________________________________________
 * |     | Packet |     |        |     |     |
 * | SOF | Type   | Len |  Data  | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B    nBytes   4B    1B
 */
typedef struct
{
  uint8_t     sof;
  uint8_t     packet_type;
  uint16_t    data_len;
  uint8_t     *data;
}__attribute__((packed)) OTA_Pra_DATA_;

/*
 * OTA Response format
 *
 * __________________________________________
 * |     | Packet |     |        |     |     |
 * | SOF | Type   | Len | Status | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B      1B     4B    1B
 */
typedef struct
{
  uint8_t   sof;
  uint8_t   packet_type;
  uint16_t  data_len;
  uint8_t   status;
  uint32_t  crc;
  uint8_t   eof;
}__attribute__((packed)) OTA_Pra_RESP_;


//Function
HAL_StatusTypeDef OTA_Write_cfg_to_flash(OTA_Pra_GNRL_CFG_ *cfg);
OTA_Pra_FunStatus OTA_Download_And_Flash( void );
void load_new_app(void);
#endif /* INC_OTA_UPLOAD_H_ */
