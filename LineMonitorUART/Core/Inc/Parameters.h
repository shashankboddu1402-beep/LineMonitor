/*
 * Parameters.h
 *
 *  Created on: Sep 7, 2025
 *      Author: fervi
 */

#ifndef INC_PARAMETERS_H_
#define INC_PARAMETERS_H_

#include "main.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_tim.h"
// Default Configuration Values
#define ADCInput_Gain 1

#define DI_channels 2
#define DO_channels 2
#define AI_channels 3
#define AO_channels 2



typedef enum
{
	ADC_GPIOInput = 1,
	ADC_GPIOOutput,
	ADC_SingleEnded,
	ADC_Differential,
	ADC_DAC
}ADCPIN_Config;


typedef struct{
	ADCPIN_Config ADCPINMode;
	uint8_t DI[DI_channels];
	uint8_t DO[DO_channels];
	float DC_RefVolt;
	float AI[AI_channels];
	uint16_t AO[AO_channels];
	uint32_t PWMI1_frq;
	uint32_t PWMI1_CycleTime;
	float PWMI1_duty;
	uint32_t PWMI2_frq;
	uint32_t PWMI2_CycleTime;
	float PWMI2_duty;
}DataPar;

//Printf
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

#endif /* INC_PARAMETERS_H_ */
