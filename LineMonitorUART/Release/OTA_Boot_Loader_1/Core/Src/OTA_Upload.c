/*
 * OTA_Upload.c
 *
 *
 *      Author: Shashank
 */

#include <stdio.h>
#include "OTA_Upload.h"

#include "main.h"
#include <string.h>
#include <stdbool.h>

extern CRC_HandleTypeDef hcrc;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

/* Buffer to hold the received data */
static uint8_t Rx_Buffer[ OTA_Pra_PACKET_MAX_SIZE ];

/* OTA State */
static OTA_Pra_STATE_ OTA_Status = OTA_Value_STATE_IDLE;



/* Firmware Size that we have received */
static uint32_t OTA_Fw_Received_Size = 0;

/* Firmware Total Size that we are going to receive */
static uint32_t OTA_Fw_Total_Size;

/* Firmware image's CRC32 */
static uint32_t OTA_Fw_CRC;




/* Configuration */
OTA_Pra_GNRL_CFG_ *cfg_flash   = (OTA_Pra_GNRL_CFG_*) (CONFIG_FLASH_ADDR);

//
static uint16_t OTA_Receiving_Chunk( uint8_t *buf, uint16_t max_len );
static HAL_StatusTypeDef OTA_Flash_Write_Data(uint8_t *data, uint32_t data_len, bool is_first_block, const uint8_t IsItApp);
static void OTA_Send_Resp( uint8_t type );
static OTA_Pra_FunStatus OTA_Process_Data( uint8_t *buf, uint16_t len );


void Clear_UART_Buffer(UART_HandleTypeDef *huart)
{
    uint8_t dummy;
    while(__HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE) != RESET)
    {
        HAL_UART_Receive(huart, &dummy, 1, 0);
    }
}

/**
  * @brief Download the application from UART and flash it.
  * @param None
  * @retval ETX_OTA_EX_
  */
OTA_Pra_FunStatus OTA_Download_And_Flash( void )
{
  OTA_Pra_FunStatus ret  = OTA_Value_Fun_OK;
  uint16_t    len = 0u;



  /* Reset the variables */
  OTA_Fw_Total_Size    = 0u;
  OTA_Fw_Received_Size = 0u;
  OTA_Fw_CRC           = 0u;
  OTA_Status            = OTA_Value_STATE_START;

  printf("Waiting for the OTA data...\r\n");
  do
  {

    //clear the buffer
    memset( Rx_Buffer, 0, OTA_Pra_PACKET_MAX_SIZE );

    len = OTA_Receiving_Chunk( Rx_Buffer, OTA_Pra_PACKET_MAX_SIZE );


    if( len != 0u )
    {
      ret = OTA_Process_Data(Rx_Buffer, len);
    }
    else
    {
      //didn't received data. break.
     printf("received data not valid \r\n");
      ret = OTA_Value_Fun_ERR;
    }

    //Send ACK or NACK
    if( ret != OTA_Value_Fun_OK )
    {
      printf("Sending NACK\r\n");
      OTA_Send_Resp( OTA_Pra_NACK );
      break;
    }
    else
    {
      printf("Sending ACK\r\n");
      OTA_Send_Resp( OTA_Pra_ACK );
    }

  }while( (OTA_Status != OTA_Value_STATE_IDLE)  );

  return ret;
}

/**
  * @brief Receive a one chunk of data.
  * @param buf buffer to store the received data
  * @param max_len maximum length to receive
  * @retval Receive Length
 */


static uint16_t OTA_Receiving_Chunk( uint8_t *buf, uint16_t max_len )
{
	printf("In OTA_Receiving_Chunk FUnction OTA_Status %d\r\n",OTA_Status);
  int16_t  ret;
  uint16_t index        = 0u;
  uint16_t data_len     = 0u;
  uint32_t cal_data_crc = 0u;
  uint32_t rec_data_crc = 0u;
  do
  {
    //receive SOF byte (1byte)
    ret = HAL_UART_Receive( &huart1, &buf[index], 1, 10000 );//id = 0
    if( ret != HAL_OK )
    {
      break;
    }
    if( buf[index] != OTA_Pra_SOF )
    {
      //Not received start of frame
      ret = OTA_Value_Fun_ERR;
      break;
    }

    //Receive the packet type (1byte).
    index++;
    ret = HAL_UART_Receive( &huart1, &buf[index], 1, 10000 );//id = 1
    if( ret != HAL_OK )
    {
      break;
    }
    index++;
    //Get the data length (2bytes).
    ret = HAL_UART_Receive( &huart1, &buf[index], 2, 10000 );//id = 2
    if( ret != HAL_OK )
    {
      break;
    }
    data_len = *(uint16_t *)&buf[index];
    index += 2u;

    for( uint16_t i = 0u; i < data_len; i++ )//id = 4
    {
      ret = HAL_UART_Receive( &huart1, &buf[index], 1, 10000 );
      index++;
      if( ret != HAL_OK )
      {
        break;
      }
    }

    if( ret != HAL_OK )
    {
      break;
    }

    //Get the CRC.
    ret = HAL_UART_Receive( &huart1, &buf[index], 4, 10000 );
    if( ret != HAL_OK )
    {
      break;
    }
    rec_data_crc = *(uint32_t *)&buf[index];
    index += 4u;

    //receive EOF byte (1byte)
    ret = HAL_UART_Receive( &huart1, &buf[index], 1, 10000 );
    if( ret != HAL_OK )
    {
      break;
    }
    if( buf[index] != OTA_Pra_EOF )
    {
      //Not received end of frame
      ret = OTA_Value_Fun_ERR;
      break;
    }
    index++;
    //Calculate the received data's CRC
    cal_data_crc = HAL_CRC_Calculate( &hcrc, (uint32_t*)&buf[4], data_len);

    //Verify the CRC
    if( cal_data_crc != rec_data_crc )
    {
      printf("Chunk's CRC mismatch [Cal CRC = 0x%08lX] [Rec CRC = 0x%08lX]\r\n",
                                                   cal_data_crc, rec_data_crc );
      ret = OTA_Value_Fun_ERR;
      break;
    }

  }while( false );
//  printf("Printing the Response ");
//  printf("%02X,",buf[0]);
//  printf("%02X,",buf[1]);
//  printf("%04X,",data_len);
//
//  for(uint16_t id = 0; id < data_len; id++)
//  {
//	  printf("%02X ",buf[id+4]);
//  }
//  printf("%08lX,",rec_data_crc);
//  printf("%02X,",buf[index-1]);
//  printf("End");
//  printf("Received Packet length %u\n",index);
  if( ret != HAL_OK )
  {
    //clear the index if error
    index = 0u;
  }

  if( max_len < index )
  {
    printf("Received more data than expected. Expected = %d, Received = %d\r\n",
                                                              max_len, index );
    index = 0u;
  }

  return index;
}

/**
  * @brief Write data to the flash location.
  * @param data data to be written
  * @param data_len data length
  * @is_first_block true - if this is first block, false - not first block
  * @retval HAL_StatusTypeDef
  */
static HAL_StatusTypeDef OTA_Flash_Write_Data(uint8_t *data, uint32_t data_len, bool is_first_block, const uint8_t IsItApp)
{
    HAL_StatusTypeDef ret;
    uint16_t retries = 0;
    uint32_t flash_address = SloatFLASH_BASE;

    if(IsItApp)
    {
    	flash_address = ApplFLASH_BASE;
    }
    else
    {
    	flash_address = SloatFLASH_BASE;
    }

    // Define flash base addresses for each slot


    do
    {
        ret = HAL_FLASH_Unlock();
        if(ret != HAL_OK)
        {
        	printf("Failed To unlock  the Flash\n");
        	break;
        }


        if(is_first_block)
        {
            // Erase Flash only for the first block
            FLASH_EraseInitTypeDef EraseInitStruct;
            uint32_t PageError;

            if(IsItApp)
            {
                EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
                EraseInitStruct.Page = ApplStartPg;
                EraseInitStruct.NbPages = ApplNbPg;
            }
            else
            {
                EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
                EraseInitStruct.Page = SloatStartPg;
                EraseInitStruct.NbPages = SloatNbPg;
            }

            ret = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
            if(ret != HAL_OK){
            	printf("Failed HAL_FLASHEx_Erase\r\n");
            	break;
            }
            else
            {
            	printf("Data Flash Erase Done\r\n");
            }
        }

        for( uint32_t i = 0; i < data_len;)
        {
//        	printf("data_len %u,i%lu \n",data_len,i);
            retries = 0;
            while(retries < max_retries)
            {
                uint64_t temp_data = 0xFFFFFFFFFFFFFFFFULL; // Padding with 0xFF

                uint32_t chunk_size = (data_len - i < 8) ? data_len - i : 8;

                // Copy data to temp_data
                memcpy(&temp_data, &data[i], chunk_size);

//                uint32_t high_data = (uint32_t)(temp_data >> 32); // High 32 bits
//                uint32_t low_data = (uint32_t)(temp_data & 0xFFFFFFFF); // Low 32 bits

                // Write double-word to flash
//                printf("Address %08lX, Data 0x%08lX%08lX, ", (flash_address + OTA_Fw_Received_Size), (uint32_t)(temp_data >> 32), (uint32_t)(temp_data & 0xFFFFFFFF));
                if(!IsItApp)
                {
                	ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flash_address + OTA_Fw_Received_Size, temp_data);
//                	printf("Sloat Address %08lX, Data %lu%lu\n", (flash_address + OTA_Fw_Received_Size),high_data,low_data);
                }
                else
                {
                	ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flash_address + i, temp_data);

//                	printf("App Address %08lX, Data %lu%lu\n", (flash_address + i),high_data,low_data);
//                	HAL_Delay(50);
                }

                if(ret == HAL_OK)
                {
                	if(!IsItApp)
                	{
                		OTA_Fw_Received_Size += chunk_size;
                	}
//                    OTA_Fw_Received_Size += chunk_size;
                    i += chunk_size;
                    break;  // Exit retry loop on success
                }
                else
                {
                	printf("Failed Data FLASH\r\n");
                    ++retries;  // Retry on failure
                }
            }

            if(retries >= max_retries)
            {
                ret = HAL_ERROR;
                printf("Failed Data FLASH Max try\r\n");
                break;  // Exit main loop on repeated failures
            }
        }

        if(ret != HAL_OK) {
        	printf("Data Flash Failed\r\n");
        	break;
        }

        ret = HAL_FLASH_Lock();
        if(ret != HAL_OK)
        {
        	printf("Failed To lock  the Flash\n");
        	break;
        }

    } while(false);

    return ret;
}

/**
  * @brief Send the response.
  * @param type ACK or NACK
  * @retval none
  */
static void OTA_Send_Resp( uint8_t type )
{
  OTA_Pra_RESP_ rsp =
  {
    .sof         = OTA_Pra_SOF,
    .packet_type = OTA_Value_PACKET_TYPE_RESPONSE,
    .data_len    = 1u,
    .status      = type,
    .eof         = OTA_Pra_EOF
  };

  // Create a temporary uint32_t variable
  uint32_t temp_crc_input = 0;
  // Copy the status byte into the least significant byte of the temporary variable
  memcpy(&temp_crc_input, &rsp.status, sizeof(rsp.status));
  // Calculate CRC
  rsp.crc = HAL_CRC_Calculate(&hcrc, &temp_crc_input, 1);

  //send response
  Clear_UART_Buffer(&huart1);
  HAL_UART_Transmit(&huart1, (uint8_t *)&rsp, sizeof(OTA_Pra_RESP_), HAL_MAX_DELAY);
//  printf("Printing the Send AKL ");
//  printf("%02X,",rsp.sof);
//  printf("%02X,",rsp.packet_type);
//  printf("%04X,",rsp.data_len);
//  printf("%02X,",rsp.status);
//  printf("%08lX,",rsp.crc);
//  printf("%02X,",rsp.eof);
//  printf("End\n");

}

/**
  * @brief Process the received data from UART4.
  * @param buf buffer to store the received data
  * @param max_len maximum length to receive
  * @retval OTA_Pra_FunStatus
  */
static OTA_Pra_FunStatus OTA_Process_Data( uint8_t *buf, uint16_t len )
{
  OTA_Pra_FunStatus ret = OTA_Value_Fun_ERR;

  do
  {
    if( ( buf == NULL ) || ( len == 0u) )
    {
      break;
    }

    //Check we received OTA Abort command
    OTA_Pra_COMMAND_ *cmd = (OTA_Pra_COMMAND_*)buf;
    if( cmd->packet_type == OTA_Value_PACKET_TYPE_CMD )
    {
      if( cmd->cmd == OTA_Value_CMD_ABORT )
      {
        //received OTA Abort command. Stop the process
        break;
      }
    }

    switch( OTA_Status )
    {
      case OTA_Value_STATE_IDLE:
      {
        printf("OTA_Value_STATE_IDLE...\r\n");
        ret = OTA_Value_Fun_OK;
      }
      break;

      case OTA_Value_STATE_START:
      {
        OTA_Pra_COMMAND_ *cmd = (OTA_Pra_COMMAND_*)buf;

        if( cmd->packet_type == OTA_Value_PACKET_TYPE_CMD )
        {
          if( cmd->cmd == OTA_Value_CMD_START )
          {
            printf("Received OTA START Command\r\n");
            OTA_Status = OTA_Value_STATE_HEADER;
            ret = OTA_Value_Fun_OK;
          }
        }
      }
      break;

      case OTA_Value_STATE_HEADER:
      {
        OTA_Pra_HEADER_ *header = (OTA_Pra_HEADER_*)buf;
        if( header->packet_type == OTA_Value_PACKET_TYPE_HEADER )
        {
          OTA_Fw_Total_Size = header->meta_data.package_size;
          OTA_Fw_CRC        = header->meta_data.package_crc;




          printf("Received OTA Header. FW Size = %lu, CRC =%08lX \r\n", OTA_Fw_Total_Size,OTA_Fw_CRC);

          //get the slot number

          OTA_Status = OTA_Value_STATE_DATA;
          ret = OTA_Value_Fun_OK;

        }
      }
      break;

      case OTA_Value_STATE_DATA:
      {

        OTA_Pra_DATA_     *data     = (OTA_Pra_DATA_*)buf;
        uint16_t          data_len = data->data_len;
        HAL_StatusTypeDef ex;

        if( data->packet_type == OTA_Value_PACKET_TYPE_DATA )
        {
          bool is_first_block = false;
          if( OTA_Fw_Received_Size == 0 )
          {
            //This is the first block
            is_first_block = true;

            /* Read the configuration */
            OTA_Pra_GNRL_CFG_ cfg;
            memcpy( &cfg, cfg_flash, sizeof(OTA_Pra_GNRL_CFG_) );

            /* Before writing the data, reset the available slot */
            cfg.slot_table.is_this_slot_not_valid = 1u;
            /* write back the updated config */

            if( OTA_Write_cfg_to_flash( &cfg ) != HAL_OK )
            {
            	ret = OTA_Value_Fun_ERR;
              printf("Config_Write error\n");
              break;
            }
//            HAL_Delay(1000);
          }

          /* write the chunk to the Flash (App location) */
          ex = OTA_Flash_Write_Data(buf+4, data_len, is_first_block, 0);

          if( ex == HAL_OK )
          {
            printf("[%ld/%ld]\r\n", OTA_Fw_Received_Size/OTA_Pra_DATA_MAX_SIZE, OTA_Fw_Total_Size/OTA_Pra_DATA_MAX_SIZE);
            if( OTA_Fw_Received_Size >= OTA_Fw_Total_Size )
            {
              //received the full data. So, move to end
            	printf("Waiting For END Command\r\n");
              OTA_Status = OTA_Value_STATE_END;
            }
            ret = OTA_Value_Fun_OK;
          }
          else
          {
        	  printf("Flash_Write error\n");
          }
        }
        else
        {
        	printf("packet_type error\n");
        }
      }
      break;

      case OTA_Value_STATE_END:
      {


        OTA_Pra_COMMAND_ *cmd = (OTA_Pra_COMMAND_*)buf;

        if( cmd->packet_type == OTA_Value_PACKET_TYPE_CMD )
        {
          if( cmd->cmd == OTA_Value_CMD_END )
          {
            printf("Received OTA END Command\r\n");

            printf("Validating the received Binary...\r\n");


            //Calculate and verify the CRC
            uint32_t cal_crc = HAL_CRC_Calculate( &hcrc, (uint32_t*)SloatFLASH_BASE, OTA_Fw_Total_Size);


            printf(" cal_crc = %08lX,OTA_Fw_CRC = %08lX\r\n",cal_crc,OTA_Fw_CRC);
            if( cal_crc != OTA_Fw_CRC )
            {
              printf("ERROR: FW CRC Mismatch cal_crc = %08lX,OTA_Fw_CRC = %08lX\r\n",cal_crc,OTA_Fw_CRC);
              break;
            }
            printf("Done!!!\r\n");

            /* Read the configuration */
            OTA_Pra_GNRL_CFG_ cfg;
            memcpy( &cfg, cfg_flash, sizeof(OTA_Pra_GNRL_CFG_) );

            //update the slot
            cfg.slot_table.fw_crc                 = cal_crc;
            cfg.slot_table.fw_size                = OTA_Fw_Total_Size;
            cfg.slot_table.is_this_slot_not_valid = 0u;
            cfg.slot_table.should_we_load_this_fw  = 1u;

            //update the reboot reason
            cfg.reboot_cause = OTA_NORMAL_BOOT;

            /* write back the updated config */

            if( OTA_Write_cfg_to_flash( &cfg ) == HAL_OK )
            {
              OTA_Status = OTA_Value_STATE_IDLE;
              ret = OTA_Value_Fun_OK;
              printf("Config Updated\r\n");
            }
            else
            {
            	printf("Config Updated failed\r\n");
            }
          }
          else
          {
        	  printf("Not Valid CMD in the END STATE\r\n");
          }
        }
        else
        {
        	printf("Not Valid packet_type in the END STATE\r\n");
        }
      }
      break;

      default:
      {
        /* Should not come here */
        ret = OTA_Value_Fun_ERR;
        printf("OTA_Value_Fun_ERR\r\n");
      }
      break;
    };
  }while( false );

  return ret;
}

/**
  * @brief Write the configuration to flash
  * @param cfg config structure
  * @retval none
  */
 HAL_StatusTypeDef OTA_Write_cfg_to_flash(OTA_Pra_GNRL_CFG_ *cfg) {
    HAL_StatusTypeDef ret;
    uint32_t address = CONFIG_FLASH_ADDR; // Set the starting flash address for configuration data

    do {
        if (cfg == NULL) {
            ret = HAL_ERROR;
            break;
        }

        ret = HAL_FLASH_Unlock();
        if (ret != HAL_OK) {
        	printf("wr: Failed to UNloack the flash\n");
            break;
        }


        // Erase the Flash
        FLASH_EraseInitTypeDef EraseInitStruct;
        uint32_t PageError;

        EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
        EraseInitStruct.Page = ConfigStartPg;
        EraseInitStruct.NbPages = ConfigNbPg;

        ret = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
        if (ret != HAL_OK) {
        	printf("Configuration Flash Erase Error\r\n");
            break;
        }


        // Write the configuration in double-word (8-byte) chunks
        uint8_t *data = (uint8_t *)cfg;
        for (uint32_t i = 0; i < sizeof(OTA_Pra_GNRL_CFG_); i += 8) {
            uint64_t temp_data = 0xFFFFFFFFFFFFFFFF;
            memcpy(&temp_data, data + i, (sizeof(OTA_Pra_GNRL_CFG_) - i >= 8) ? 8 : (sizeof(OTA_Pra_GNRL_CFG_) - i));
            ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address + i, temp_data);
            if (ret != HAL_OK) {
                printf("Configuration Flash Write Error\r\n");
                break;
            }
        }

        if (ret != HAL_OK) {
        	printf("Wr: Failed to flash the config\r\n");
            break;
        }

        ret = HAL_FLASH_Lock();

        if (ret != HAL_OK) {
        	printf("wr: Failed to loack the flash\n");
            break;
        }
    } while (false);

    return ret;
}

/**
 * @brief Load the new app to the app's actual flash memory.
 * @param none
 * @retval none
 */
void load_new_app(void)
{

  printf("Loading Application\r\n");
  HAL_StatusTypeDef ret;

  /* Read the configuration */

  OTA_Pra_GNRL_CFG_ cfg;
  memcpy(&cfg, cfg_flash, sizeof(OTA_Pra_GNRL_CFG_));
  printf("1\n");

  printf("fw_crc = %lu fw_size = %lu ",cfg.slot_table.fw_crc,cfg.slot_table.fw_size);

  if (cfg.slot_table.should_we_load_this_fw == 1u)
  {
//	  uint32_t address = SloatFLASH_BASE;

	  // Verify the application is corrupted or not
	  printf("Verifying the Application...");

	  FLASH_WaitForLastOperation(5000);
	  printf("2\n");

	  uint32_t cal_data_crc = 0;

	  // Verify the application
	  if(cfg.slot_table.fw_size < 106000)
	  {
		  cal_data_crc = HAL_CRC_Calculate(&hcrc, (uint32_t *)SloatFLASH_BASE, cfg.slot_table.fw_size);
		  printf("SloatFLASH_BASE CRC = %lu\n",cal_data_crc);
	  }
	  FLASH_WaitForLastOperation(5000);

	  if(cfg.slot_table.fw_crc == cal_data_crc)
	  {
		    // Load the new app or firmware to app's flash address
			printf("\t<< Load the new firmware to app's flash address >>\n");
			ret = OTA_Flash_Write_Data((uint8_t*)SloatFLASH_BASE, cfg.slot_table.fw_size, 1,1);
			if( ret != HAL_OK )
			{
				printf("App Flash write Error\r\n");
			}
			else
			{
				printf("App Flash write Done\\r\n");
			}

		    /* write back the updated config */
		    cfg.slot_table.should_we_load_this_fw = 0u;
		    ret = OTA_Write_cfg_to_flash(&cfg);
		    if (ret != HAL_OK)
		    {
		      printf("Config Flash write Error\r\n");
		    }
	  }
	  else
	  {
		  printf("CRC mismatch\n");
	  }
  }
  printf("3\n");
  // Verify the application is corrupted or not
  printf("Verifying the Application...");

  FLASH_WaitForLastOperation(10000);
  printf("4\n");

  uint32_t cal_data_crc = 0;

  // Verify the application
  if(cfg.slot_table.fw_size < 106000)
  {
	  cal_data_crc = HAL_CRC_Calculate(&hcrc, (uint32_t *)ApplFLASH_BASE, cfg.slot_table.fw_size);
	  printf("ApplFLASH_BASE CRC = %lu\n",cal_data_crc);
  }
  FLASH_WaitForLastOperation(10000);
  printf("5\n");

  // Verify the CRC
  if (cal_data_crc != cfg.slot_table.fw_crc)
  {
    printf("ERROR!!!\r\n");
    printf("Invalid Application. HALT!!!\r\n");

  }
  printf("Done!!!\r\n");
}
