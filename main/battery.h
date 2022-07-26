/*
 * battery.h
 *
 *  Created on: Jul 7, 2022
 *      Author: Lenovo
 */

#ifndef MAIN_BATTERY_H_
#define MAIN_BATTERY_H_

#include "driver/adc.h"

typedef enum
{
	BATT_OCV_SOC,
	BATT_COULOMB_COUNTING
} counting_method_t;

typedef struct
{
	float SoC;
	float SoH;
	uint16_t cycle_count;
	uint8_t cycle_mem;
	float capacity;
	uint32_t status;
} battery_param_t;

float BAT_SOCCount(float voltage, float current, battery_param_t *param, counting_method_t method);
uint32_t BAT_FloatToHex(float value);

#endif /* MAIN_BATTERY_H_ */
