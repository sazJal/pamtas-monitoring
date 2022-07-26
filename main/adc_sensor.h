/*
 * adc_sensor.h
 *
 *  Created on: Jun 20, 2022
 *      Author: Lenovo
 */

#ifndef MAIN_ADC_SENSOR_H_
#define MAIN_ADC_SENSOR_H_

#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define TIMES              512 // 4 sensor x 2 bytes x 32 samples
#define GET_UNIT(x)        ((x>>3) & 0x1)

#define ADC_RESULT_BYTE     2
#define ADC_CONV_LIMIT_EN   1                       	//For ESP32, this should always be set to 1
#define ADC_CONV_MODE       ADC_CONV_SINGLE_UNIT_1	  	//ESP32 only supports ADC1 DMA mode
#define ADC_OUTPUT_TYPE     ADC_DIGI_OUTPUT_FORMAT_TYPE1

esp_adc_cal_characteristics_t adc1_chan_0_chars;
esp_adc_cal_characteristics_t adc1_chan_3_chars;
esp_adc_cal_characteristics_t adc1_chan_6_chars;
esp_adc_cal_characteristics_t adc1_chan_7_chars;

uint16_t adc1_chan_mask;
uint16_t adc2_chan_mask;
adc_channel_t channel[4];

uint32_t v_pv32, v_batt32, i_pv32, i_batt32;


void continuous_adc_init(uint16_t adc1_chan_mask, uint16_t adc2_chan_mask, adc_channel_t *channel, uint8_t channel_num);
bool check_valid_data(const adc_digi_output_data_t *data);



#endif /* MAIN_ADC_SENSOR_H_ */
