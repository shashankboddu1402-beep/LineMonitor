/*
 * ReadAction.c
 *
 *  Created on: Sep 6, 2025
 *      Author: Shashank
 */

#include "ReadAction.h"
			/*Micro*/


			/*Variables*/
//Handles
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim4;
//
extern DataPar DataVar;

uint32_t ADCChannels[] = {ADC_CHANNEL_1,ADC_CHANNEL_2,ADC_CHANNEL_3,ADC_CHANNEL_4};
			/*Interrupt Function */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
        {
        	DataVar.PWMI1_CycleTime =  HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

        }
        else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
        {
        	DataVar.PWMI1_duty = (HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2)*100.0)/DataVar.PWMI1_CycleTime;
        	uint32_t TIM1Frq = 1;// Get the Frq of the timer
        	DataVar.PWMI1_frq = (TIM1Frq/DataVar.PWMI1_CycleTime);
        }
    }
    if (htim->Instance == TIM4)
    {
        if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
        {
        	DataVar.PWMI2_CycleTime =  HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

        }
        else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
        {
        	DataVar.PWMI2_duty = (HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2)*100)/DataVar.PWMI2_CycleTime;
        	uint32_t TIM4Frq = 1;// Get the Frq of the timer
        	DataVar.PWMI2_frq = (TIM4Frq/DataVar.PWMI2_CycleTime);
        }
    }
}
		/*Function Declaration*/

//ADC
static uint16_t ADC_init_Read(const uint32_t Channel,ADC_HandleTypeDef *hADC );
static float Get_ADCVref();

		/*Function Definition*/
void Read_InputPins(DataPar* DataPtr)
{
	printf("\t/* Reading the Input Pins\t\n");
	static uint32_t InloopTime = 0;
	if(!InloopTime)
	{
		HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1);   // main channel
		HAL_TIM_IC_Start(&htim1, TIM_CHANNEL_2);   // indirect channel

		HAL_TIM_IC_Start_IT(&htim4, TIM_CHANNEL_1);   // main channel
		HAL_TIM_IC_Start(&htim4, TIM_CHANNEL_2);   // indirect channel
	}
	//Digital Read
	DataPtr->DI[0] = HAL_GPIO_ReadPin(DI1_GPIO_Port, DI1_Pin);
	DataPtr->DI[1] = HAL_GPIO_ReadPin(DI2_GPIO_Port, DI2_Pin);
	//ADC read of Single ended
	if(((HAL_GetTick()-InloopTime) > 300000) ||(!InloopTime))
	{
		//Read the DC reference voltages
		DataPtr->DC_RefVolt = Get_ADCVref();
		InloopTime = HAL_GetTick();
	}
	DataPtr->AI[0] = ADC_init_Read(ADCChannels[2], &hadc1);
	DataPtr->AI[1] = ADC_init_Read(ADCChannels[3], &hadc1);
	DataPtr->AI[2] = ADC_init_Read(ADCChannels[0], &hadc2);
	DataPtr->AI[2] = ADC_init_Read(ADCChannels[1], &hadc2);
	DataPtr->AI[2] = ADC_init_Read(ADCChannels[2], &hadc2);
	DataPtr->AI[2] = ADC_init_Read(ADCChannels[3], &hadc2);
	//PWM inputRead

}


//ADC
//
static uint16_t ADC_init_Read(const uint32_t Channel,ADC_HandleTypeDef *hADC )
{
	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = Channel;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_24CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;
	if (HAL_ADC_ConfigChannel(hADC, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}

//	HAL_ADCEx_Calibration_Start(hADC,ADC_SINGLE_ENDED );
	HAL_ADC_Start(hADC);
	HAL_ADC_PollForConversion(hADC, 1000);
	uint16_t ADC_Value = HAL_ADC_GetValue(hADC);
	HAL_ADC_Stop(hADC);
	return ADC_Value;
}

static float Get_ADCVref()
{
	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = ADC_CHANNEL_VREFINT;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_92CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}

	float Vref = 3.3;
	HAL_Delay(10);
	HAL_ADCEx_Calibration_Start(&hadc1,ADC_SINGLE_ENDED );
	HAL_Delay(10);
	if (HAL_ADC_Start(&hadc1) != HAL_OK) {
		return Vref;
	}
	if (HAL_ADC_PollForConversion(&hadc1, 1000) != HAL_OK) {
		return Vref;
	}
	Vref = HAL_ADC_GetValue(&hadc1);
	if (HAL_ADC_Stop(&hadc1) != HAL_OK) {
		return 3.3f;
	}
	Vref *= 1000;
	Vref = (VREFINT_CAL_VREF * *VREFINT_CAL_ADDR)/Vref;
	return Vref;
}
