/*
 * OTA_Flash_Config.h
 *
 *  Created on: Nov 28, 2023
 *      Author: fervi
 */

#ifndef INC_OTA_FLASH_CONFIG_H_
#define INC_OTA_FLASH_CONFIG_H_

//Slot -1 Pages
#define ApplStartPg 22
#define ApplNbPg  53
#define ApplFLASH_BASE 0x0800B000

//Slot -2 Pages
#define SloatStartPg 75
#define SloatNbPg  52
#define SloatFLASH_BASE  0x08025800

//Config Pages
#define CONFIG_FLASH_ADDR  0x0800A800
#define ConfigStartPg 21
#define  ConfigNbPg  1

#define max_retries 10 // Maximum number of retries to write in flash

#endif /* INC_OTA_FLASH_CONFIG_H_ */



