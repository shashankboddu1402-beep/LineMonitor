/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "BooTLoad_Flash_Config.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
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

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t BootLoaderVersion = 2;

uint32_t OTA_Fw_Received_Size = 0;
OTA_Pra_GNRL_CFG_ *cfg_flash   = (OTA_Pra_GNRL_CFG_*) (CONFIG_FLASH_ADDR);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_CRC_Init(void);
/* USER CODE BEGIN PFP */
typedef void (*pFunction)(void);
static void goto_application(void);
void deinitEverything();

PUTCHAR_PROTOTYPE {
	HAL_UART_Transmit(&huart2, (uint8_t*) &ch, 1, 1000);
	return ch;
}

HAL_StatusTypeDef OTA_Flash_Write_Data(uint8_t *data, uint32_t data_len, uint8_t is_first_block, const uint8_t IsItApp);
HAL_StatusTypeDef OTA_Write_cfg_to_flash(OTA_Pra_GNRL_CFG_ *cfg);
void load_new_app(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */

	printf("Starting BootLoader(%d)\r\n", BootLoaderVersion);

	//Read the reboot cause and act accordingly
	printf("Reading the reboot reason...\r\n");



	load_new_app();
	goto_application();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		printf(">");
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK3|RCC_CLOCKTYPE_HCLK
                              |RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK3Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */


void deinitEverything() {
	// Deinitialize CRC peripheral
	HAL_CRC_DeInit(&hcrc);

	// Deinitialize UART peripherals

	HAL_UART_DeInit(&huart2);
	// Deinitialize all used GPIOs
	// Replace GPIOx and pin numbers with actual GPIO ports and pins used in your application

//    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_Y); // Replace GPIO_PIN_Y with actual pin numbers
			// Add more HAL_GPIO_DeInit() calls for other GPIOs if necessary

	// Disable GPIO clocks
	__HAL_RCC_GPIOA_CLK_DISABLE();
	__HAL_RCC_GPIOB_CLK_DISABLE();
	// Disable other GPIO clocks if used

	// Reset the RCC (Reset and Clock Control) to default values
	HAL_RCC_DeInit();

	// Deinitialize the HAL
	HAL_DeInit();

	// Reset SysTick Timer
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;

	// Other deinitializations if any (e.g., disabling other peripherals, resetting specific configurations, etc.)
}

static void goto_application(void) {

	printf("Gonna Jump to Application %08X\r\n", ApplFLASH_BASE);

	uint32_t JumpAddress;
	pFunction Jump_To_Application;

	printf("Gonna Jump to Application1\r\n");

	deinitEverything();

	//jump to the application

	JumpAddress = *(uint32_t*) ((ApplFLASH_BASE) + 4);
	Jump_To_Application = (pFunction) JumpAddress;
	//initialize application's stack pointer

	__set_MSP(*(uint32_t*) (ApplFLASH_BASE));
	Jump_To_Application();
}

/**
  * @brief Write data to the flash location.
  * @param data data to be written
  * @param data_len data length
  * @is_first_block 1 - if this is first block, 0 - not first block
  * @retval HAL_StatusTypeDef
  */
HAL_StatusTypeDef OTA_Flash_Write_Data(uint8_t *data, uint32_t data_len, uint8_t is_first_block, const uint8_t IsItApp)
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

    } while(0);

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
    } while (0);

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


/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
