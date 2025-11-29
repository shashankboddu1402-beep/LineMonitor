/*
 * WriteAction.c
 *
 *  Created on: Sep 9, 2025
 *      Author: Shashank
 */

#include "WriteAction.h"
#include "Parameters.h"


extern TIM_HandleTypeDef htim2;  // Assume TIM2 is initialized in PWM mode
extern TIM_HandleTypeDef htim3;

extern DataPar DataVar;

void Write_DigitalPins(DigitalPin_Set DI1,DigitalPin_Set DI2)
{
	if(DI1 != DigitalPin_NotChange)
	{
		HAL_GPIO_WritePin(DI1_GPIO_Port, DI1_Pin, DI1);
		DataVar.DI[0] = DI1;
	}

	if(DI2 != DigitalPin_NotChange)
	{
		HAL_GPIO_WritePin(DI2_GPIO_Port, DI2_Pin, DI2);
		DataVar.DI[1] = DI2;
	}

}

/**
 * @brief Set PWM frequency and duty cycle dynamically (TIM2_CH3 or TIM3_CH3)
 * @param PWMI     1 = TIM2_CH3, 2 = TIM3_CH3
 * @param freq_hz  Desired frequency (0.5 Hz - 50 Hz)
 * @param duty     Duty cycle percentage (0 - 100)
 */
void Write_PWMPins(uint8_t PWMI, float freq_hz, float duty)
{
    TIM_HandleTypeDef *htim = NULL;
    uint32_t channel = 0;

    if (PWMI == 1) {
        htim = &htim2;
        channel = TIM_CHANNEL_3;
    } else if (PWMI == 2) {
        htim = &htim3;
        channel = TIM_CHANNEL_3;
    } else {
        return; // invalid input
    }

    // --- Step 1: Get timer clock frequency ---
    uint32_t timer_clk;
    if ((htim->Instance == TIM1) || (htim->Instance == TIM8)) {
        timer_clk = HAL_RCC_GetPCLK2Freq();
        if ((RCC->CFGR & RCC_CFGR_PPRE2) != RCC_CFGR_PPRE2_DIV1) {
            timer_clk *= 2;
        }
    } else {
        timer_clk = HAL_RCC_GetPCLK1Freq();
        if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1) {
            timer_clk *= 2;
        }
    }

    // --- Step 2: Compute ARR & Prescaler ---
    uint32_t period = (uint32_t)(timer_clk / freq_hz);
    uint32_t prescaler = (period / 65535) + 1;   // keep ARR <= 65535
    uint32_t arr = (timer_clk / (prescaler * freq_hz)) - 1;

    // --- Step 3: Update registers (glitch-free with preload) ---
    __HAL_TIM_DISABLE(htim);

    htim->Instance->PSC = prescaler - 1;
    htim->Instance->ARR = arr;

    uint32_t ccr = (uint32_t)((duty / 100.0f) * (arr + 1));
    __HAL_TIM_SET_COMPARE(htim, channel, ccr);

    // Generate an update event to load PSC/ARR/CCR into shadow registers
    htim->Instance->EGR = TIM_EGR_UG;

    __HAL_TIM_ENABLE(htim);
}

/**
 * @brief Write analog voltage to DAC pin (Controllerstech-style)
 * @param AO: 1 or 2
 * @param voltage: Desired output in volts (0.0 to Vref, e.g. 3.3V)
 */
void Write_AOPin(uint32_t AO,float voltage)
{
	uint32_t channel = NULL;
    const float VREF = DataVar.DC_RefVolt;
    uint32_t dac_val = (uint32_t)(voltage * 4096.0f / VREF);

    if (dac_val > 4095) {
        dac_val = 4095;
    }

    if (AO == 1) {
    	DataVar.AO[0] = dac_val;
        channel = DAC_CHANNEL_1;
    } else if (AO == 2) {
    	DataVar.AO[1] = dac_val;
        channel = DAC_CHANNEL_2;
    } else {
        return; // invalid input
    }


    HAL_DAC_SetValue(&hdac1, channel, DAC_ALIGN_12B_R, dac_val);
}
