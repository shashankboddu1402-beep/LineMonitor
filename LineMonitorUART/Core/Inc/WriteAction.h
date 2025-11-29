/*
 * WriteAction.h
 *
 *  Created on: Sep 9, 2025
 *      Author: Shashank
 */

#ifndef INC_WRITEACTION_H_
#define INC_WRITEACTION_H_

typedef enum {
	DigitalPin_Low,
	DigitalPin_High,
	DigitalPin_NotChange
}DigitalPin_Set;

void Write_DigitalPins(DigitalPin_Set DI1,DigitalPin_Set DI2);
void Write_PWMPins(uint8_t PWMI, float freq_hz, float duty);//PWM_Set(1, 10.0f, 25.0f);   // TIM2_CH3, 10 Hz, 25% duty

#endif /* INC_WRITEACTION_H_ */
