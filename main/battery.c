/*
 * battery.c
 *
 *  Created on: Jul 7, 2022
 *      Author: Lenovo
 */

#include "battery.h"

float BAT_SOCCount(float voltage, float current, battery_param_t *param, counting_method_t method)
{
	float SoC=0, SoC_t;
	float capConst;
	static const float internal_resistance = 0.0054;
	float ocv;

	/* Measure OCV */
	ocv = voltage - (current*internal_resistance);

	switch(method)
	{
		case BATT_OCV_SOC			:
			if(ocv < 18.6)
			{
				SoC = 0.0;
			}
			else if((ocv >= 18.6) && (ocv < 24.0))
			{
				SoC = 3.2265*ocv - 60.876;
			}
			else if((ocv >= 24.0) && (ocv < 24.38))
			{
				SoC = 49.308*ocv - 1162.2;
			}
			else if((ocv >= 24.38) && (ocv < 24.58))
			{
				SoC = 101.2*ocv - 2426.5;
			}
			else if((ocv >= 24.58) && (ocv < 24.78))
			{
				SoC = 100*ocv - 2397;
			}
			else if((ocv >= 24.78) && (ocv < 26.6))
			{
				SoC = 9.424*ocv - 148.13;
			}

			/* When some anomaly happened */
			else if((ocv >= 26.6) || (SoC > 100.0))
			{
				SoC = 100.0;
			}
			break;
		case BATT_COULOMB_COUNTING	:
			/* Compute SoC */
			param->SoH 	 = (-0.0003*param->cycle_count) + 1.0;
			capConst  	 = param->SoH*param->capacity*36; // 100/3600 = 1/36
			param->SoC	+= (current/capConst);

			/* Compute the cycle */
			SoC_t		 = current/capConst; // metric for cycle counting
			SoC_t < 0.0? (param->cycle_mem = param->cycle_mem - SoC_t):(param->cycle_mem = param->cycle_mem + SoC_t); // keeping value positive
			if(param->cycle_mem >= 200.0) // when batteries already stored/consumed 200 % capacity
			{
				param->cycle_count += 1; // add battery cycle
				param->cycle_mem = 0.0;  // restart the counting
			}

			SoC = param->SoC;
			/* Update value to EEPROM */
			/**************************/
			break;
		default : break;
	}

	return SoC;
}

uint32_t BAT_FloatToHex(float value)
{
   unsigned char hexVals[sizeof(float)];
   uint32_t hex_data;

   hexVals[0] = ((unsigned char*)&value)[0];
   hexVals[1] = ((unsigned char*)&value)[1];
   hexVals[2] = ((unsigned char*)&value)[2];
   hexVals[3] = ((unsigned char*)&value)[3];

   hex_data = *((uint32_t*)hexVals);

   return hex_data;
}
