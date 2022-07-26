/*
 * adc_sensor.c
 *
 *  Created on: Jun 20, 2022
 *      Author: Lenovo
 */

#include "adc_sensor.h"

static const char *TAG = "ADC DMA";

esp_adc_cal_characteristics_t adc1_chan_0_chars;
esp_adc_cal_characteristics_t adc1_chan_3_chars;
esp_adc_cal_characteristics_t adc1_chan_6_chars;
esp_adc_cal_characteristics_t adc1_chan_7_chars;

uint16_t adc1_chan_mask = BIT(0) | BIT(3) | BIT(6) | BIT(7);
uint16_t adc2_chan_mask = 0;
adc_channel_t channel[4] = {ADC1_CHANNEL_0, ADC1_CHANNEL_3, ADC1_CHANNEL_6, ADC1_CHANNEL_7};

uint32_t v_pv32 = 0, v_batt32 = 0, i_pv32 = 0, i_batt32 = 0;


void continuous_adc_init(uint16_t adc1_chan_mask, uint16_t adc2_chan_mask, adc_channel_t *channel, uint8_t channel_num)
{
    adc_digi_init_config_t adc_dma_config = {
        .max_store_buf_size = 1024,
        .conv_num_each_intr = TIMES,
        .adc1_chan_mask = adc1_chan_mask,
        .adc2_chan_mask = adc2_chan_mask,
    };
    ESP_ERROR_CHECK(adc_digi_initialize(&adc_dma_config));

    adc_digi_configuration_t dig_cfg = {
        .conv_limit_en = ADC_CONV_LIMIT_EN,
        .conv_limit_num = 250,
        .sample_freq_hz = 10 * 1000,
        .conv_mode = ADC_CONV_MODE,
        .format = ADC_OUTPUT_TYPE,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = channel_num;
    for (int i = 0; i < channel_num; i++) {
        uint8_t unit = GET_UNIT(channel[i]);
        uint8_t ch = channel[i] & 0x7;
        adc_pattern[i].atten = ADC_ATTEN_DB_11;
        adc_pattern[i].channel = ch;
        adc_pattern[i].unit = unit;
        adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

        ESP_LOGI(TAG, "adc_pattern[%d].atten is :%x", i, adc_pattern[i].atten);
        ESP_LOGI(TAG, "adc_pattern[%d].channel is :%x", i, adc_pattern[i].channel);
        ESP_LOGI(TAG, "adc_pattern[%d].unit is :%x", i, adc_pattern[i].unit);
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_digi_controller_configure(&dig_cfg));
}

bool check_valid_data(const adc_digi_output_data_t *data)
{
    const unsigned int unit = data->type2.unit;
    if (unit > 2) return false;
    if (data->type2.channel >= SOC_ADC_CHANNEL_NUM(unit)) return false;

    return true;
}

//void check_efuse(void)
//{
////	//Check if TP is burned into eFuse
////	if(esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
////	{
////		printf("eFuse Two Point: Supported\n");
////	}
////	else
////	{
////	     printf("eFuse Two Point: NOT supported\n");
////	}
//	//Check Vref is burned into eFuse
//	if(esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK)
//	{
//		printf("eFuse Vref: Supported\n");
//        esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc1_chars);
//        esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc2_chars);
//	}
//	else
//	{
//		printf("eFuse Vref: NOT supported\n");
//	}
//
//	ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
//	ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_EXAMPLE_CHAN0, ADC_ATTEN_DB_11));
//	//ADC2 config
//	ESP_ERROR_CHECK(adc2_config_channel_atten(ADC2_EXAMPLE_CHAN0, ADC_ATTEN_DB_11));
//}
//
//void adc_init(void)
//{
//	check_efuse();
//}
