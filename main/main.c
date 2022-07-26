#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
//#include "nvs_flash.h"
#include "hal/uart_ll.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"

#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "driver/timer.h"
#include "driver/i2c.h"

#include "monitor.h"
#include "mcp23017.h"
#include "spi_master_nodma.h"
#include "gps.h"
#include "pzem_mux.h"
#include "tft.h"
#include "tftfunc.h"
#include "ili9488.h"
#include "string.h"
#include "adc_sensor.h"

uint32_t signed_energy_limit = 0;
float float_energy_limit;
float float_sisa;
float float_SoC_energy;
float float_kuota_energy=2000.0;

/* notes */
/* energy limit tidak boleh lebih dari kuota energy
 *
 *
 *
 *
 */
//#define TIMER_DIVIDER	(16) // 16 bit divider
//#define TIMER_SCALE		(TIMER_BASE_CLK/TIMER_DIVIDER)

TaskHandle_t ISR=NULL;
uint64_t _lastRead;
//static intr_handle_t handle_console;

//void ili_init(spi_nodma_device_handle_t spi);
//void commandList(spi_nodma_device_handle_t spi, const uint8_t *addr);
//void display_test(spi_nodma_device_handle_t spi, spi_nodma_device_handle_t tsspi);

typedef struct
{
	int timer_group;
	int timer_idx;
	int alarm_interval;
	bool auto_reload;
} example_timer_info_t;

typedef struct
{
	example_timer_info_t info;
	uint64_t timer_counter_value;
} example_timer_event_t;

#define BLINK_GPIO GPIO_NUM_2
bool work_to_be_done = false;

esp_err_t ret;
uint32_t ret_num = 0;
uint8_t result[TIMES] = {0};

//static const BaseType_t app_cpu = 0;	// if unicore
//static const BaseType_t app_cpu = 1;	// if dualcore

//uint8_t 	rxbuff[256];
//uint16_t	urxlen;
//QueueHandle_t xStringQueue;
//#define NOTASK 1

void blink_task(void *pvParameter)
{
//	static uint8_t data[3] = "A?V";
	static bool led_status = true;
	while(1)
	{
//		MON_WriteStream(data, 3);
		led_status = !led_status;
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void write_uart(void *pvParameter)
{
	uint8_t count=0;
	static uint8_t data;

	while(1)
	{
		data = mcp23017_readRegister(MCP23017_GPIOA);

		if(count==4)
		{
			count = 0;
		}
		else count++;

		switch(count)
		{
			case 0 : mcp23017_writeRegister(MCP23017_GPIOB, 0x00);
				break;
			case 1 : mcp23017_writeRegister(MCP23017_GPIOB, 0x10);
				break;
			case 2 : mcp23017_writeRegister(MCP23017_GPIOB, 0x20);
				break;
			case 3 : mcp23017_writeRegister(MCP23017_GPIOB, 0x30);
				break;
			default : break;
		}
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
}

void task_read_button(void *pvParameter)
{
	static uint8_t data;
	while(1)
	{
		data = mcp23017_readRegister(MCP23017_GPIOA);
		if(data != 0)
		{
			MON_WriteStream(&data, 1);
		}
		vTaskDelay(50 / portTICK_PERIOD_MS);
	}
}

void task_read_adc(void *pvParameter)
{
	uint32_t vpv_acc = 0, vbatt_acc = 0, ipv_acc = 0, ibat_acc = 0;

//	char buffADC[50];
	while(1)
	{
		adc_digi_start();
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		ret = adc_digi_read_bytes(result, TIMES, &ret_num, ADC_MAX_DELAY);
		if (ret == ESP_OK || ret == ESP_ERR_INVALID_STATE)
		{
//			ESP_LOGI("TASK:", "ret is %x, ret_num is %d", ret, ret_num);
			for (int i = 0; i < ret_num; i += ADC_RESULT_BYTE)
			{
				adc_digi_output_data_t *p = (void*)&result[i];
				switch(p->type1.channel)
				{
					case 0 : vpv_acc += p->type1.data;
							 break;
					case 3 : ipv_acc += p->type1.data;
							 break;
					case 6 : vbatt_acc += p->type1.data;
							 break;
					case 7 : ibat_acc += p->type1.data;
							 break;
					default : break;
				}
			}
			v_pv32 		= esp_adc_cal_raw_to_voltage((vpv_acc >> 6), &adc1_chan_0_chars); // 8
			vpv_acc 	= 0;
			i_pv32 		= esp_adc_cal_raw_to_voltage((ipv_acc >> 6), &adc1_chan_3_chars);
			ipv_acc 	= 0;
			v_batt32	= esp_adc_cal_raw_to_voltage((vbatt_acc >> 6), &adc1_chan_6_chars);
			vbatt_acc 	= 0;
			i_batt32	= esp_adc_cal_raw_to_voltage((ibat_acc >> 6), &adc1_chan_7_chars);
			ibat_acc 	= 0;
			source.pv_voltage 	= 0.0308*(v_pv32/1000.0);
			source.batt_voltage	= 0.092*(v_batt32/1000.0);
			source.pv_current	= 41.322*(i_pv32/1000.0);
			source.batt_current	= 18.182*(i_batt32/1000.0 + 1.65);
		}

	}
}

void task_pzem_received(void *pvParameter)
{
	const uart_port_t uart_num = UART_NUM_2;
	uint8_t data[128];
	int length = 0;
	while(1)
	{
		ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t*)&length));
		length = uart_read_bytes(uart_num, data, length, 100);
		if(length > 0)
		{
			pzem_data = PZEMMux_ParseData(data, length);
		}
		vTaskDelay(50 / portTICK_PERIOD_MS);
	}
}

void task_pzem_request(void *pvParameter)
{
	static channel_select_t prev_channel = PZEM_CHAN_DC_POWER;
	const uart_port_t uart_num = UART_NUM_2;
	uint8_t data[128];
	static uint8_t data_button;
	int length = 0;
	while(1)
	{
		/* read uart receive */
		ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t*)&length));
		length = uart_read_bytes(uart_num, data, length, 100);
		if(length > 0)
		{
			pzem_data = PZEMMux_ParseData(data, length);
		}

		/* read button */
		data_button = mcp23017_readRegister(MCP23017_GPIOA);
		if(data_button != 0)
		{
			switch(data_button)
			{
				case MCP23017_MASK_BUTTON_OK	:	// OK
					btn_status.isButtonOkPressed 	= true;
					break;
				case MCP23017_MASK_BUTTON_DOWN	:	// DOWN
					btn_status.isButtonDownPressed = true;
					break;
				case MCP23017_MASK_BUTTON_BACK	: 	// EDIT
					btn_status.isButtonBackPressed	= true;
					break;
				case MCP23017_MASK_BUTTON_UP	:	// UP
					btn_status.isButtonUpPressed	= true;
					break;
				case MCP23017_MASK_BUTTON_EDIT	:
					btn_status.isButtonEditPressed	= true;
					break;
				default :
					break;
			}
			MON_WriteStream(&data_button, 1);
		}


		/* send uart */
		switch(prev_channel)
		{
			case PZEM_CHAN_AC_POWER_1 : // PZEM KANAN
				/* copy buffer to corresponding data variables if valid */
				if(pzem_data.data_is_valid)
				{
					pzem.data[0] = pzem_data;
				}
				else
				{
					pzem.data[0] = PZEMMux_ClearData();
				}
				/* set mux to enable channeling for AC load 1 reading via mcp23018 */
				mcp23017_writeRegister(MCP23017_GPIOB, 0x01);
				pzem.chn = PZEM_CHAN_AC_POWER_2;
				break;
			case PZEM_CHAN_AC_POWER_2 : // PZEM TENGAH
				/* copy buffer to corresponding data variables if valid */
				if(pzem_data.data_is_valid)
				{
					pzem.data[1] = pzem_data;
				}
				else
				{
					pzem.data[1] = PZEMMux_ClearData();
				}
				/* set mux to enable channeling for AC load 2 reading via mcp23018 */
				mcp23017_writeRegister(MCP23017_GPIOB, MCP23017_PZEM_SEL_0);
//				pzem.chn = PZEM_CHAN_DC_POWER;
				pzem.chn = PZEM_CHAN_AC_POWER_1;
				break;
			case PZEM_CHAN_DC_POWER :
				/* copy buffer to corresponding data variables if valid */
				if(pzem_data.data_is_valid)
				{
					pzem.data[2] = pzem_data;
				}
				else
				{
					pzem.data[2] = PZEMMux_ClearData();
				}
				/* set mux to enable channeling for DC load reading via mcp23018 */
				mcp23017_writeRegister(MCP23017_GPIOB, MCP23017_PZEM_SEL_1);
				pzem.chn = PZEM_CHAN_AC_POWER_1;
				break;
		}

		prev_channel = pzem.chn;
		uart_flush(UART_NUM_2);
		pzem_addr = PZEM_DEFAULT_ADDRESS;
		pzem_data.data_is_valid = false;
		PZEMMux_SendCommand(PZEM_CMD_RIR, pzem_addr, 0x0000, 0x000A);

//		PZEMMux_SendCommand(PZEM_CMD_RHR, pzem_addr, WREG_ADDR, 0x0001);
		vTaskDelay(200 / portTICK_PERIOD_MS);
	}
}

void task_show_pzem(void *pvParameter)
{

	while(1)
	{
		switch(disp_state)
		{
			case DISP_FIRST_PAGE_MAIN			:
				show_first_page(pzem, float_energy_limit, float_sisa, source);
				if(btn_status.isButtonEditPressed)
				{
					disp_state = DISP_FIRST_PAGE_EDIT_SELECT;
				}
				else if(btn_status.isButtonUpPressed || btn_status.isButtonDownPressed)
				{
					clean_first_page();
					draw_second_page();
					disp_state = DISP_SECOND_PAGE;
				}
				break;
			case DISP_FIRST_PAGE_EDIT_SELECT 	:
				show_first_page(pzem, float_energy_limit, float_sisa, source);
				draw_up_triangle();
				draw_down_triangle();
				draw_edit_box();
				if(btn_status.isButtonBackPressed)
				{
					/* clear hover box */
					erase_edit_box();
					erase_up_triangle();
					erase_down_triangle();
					/* switch to other state */
					disp_state = DISP_FIRST_PAGE_MAIN;
				}
				else if(btn_status.isButtonOkPressed)
				{
					/* clear hover box */
					erase_edit_box();
					erase_up_triangle();
					erase_down_triangle();
					/* switch to other state */
					disp_state = DISP_FIRST_PAGE_MAIN;
				}
				else if(btn_status.isButtonUpPressed)
				{
					float_energy_limit += 50.0;
					if(float_energy_limit >= 13000.0)
					{
						float_energy_limit = 13000.0;
					}

				}
				else if(btn_status.isButtonDownPressed)
				{
					float_energy_limit -= 50.0;
					if(float_energy_limit <= 0.0)
					{
						float_energy_limit = 0.0;
					}
				}
				break;
			case DISP_SECOND_PAGE				:
				show_second_page(pzem, source);
				if(btn_status.isButtonUpPressed || btn_status.isButtonDownPressed)
				{
					clean_second_page();
					draw_first_page();
					disp_state = DISP_FIRST_PAGE_MAIN;
				}
				break;
		}
		clear_button_status(&btn_status);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void app_main(void)
{
	source.batt_current = 0.0;
	source.batt_power	= 0.0;
	source.batt_voltage = 0.0;
	source.pv_current	= 0.0;
	source.pv_power		= 0.0;
	source.pv_voltage	= 0.0;

	MON_Init();
	memset(result, 0xcc, TIMES); // used for ADC purpose
	PZEMMux_Init(&pzem);
	pzem_data.data_is_valid = false;
	float_energy_limit = 40.0;
	disp_state	= DISP_FIRST_PAGE_MAIN;

	clear_button_status(&btn_status);

	mcp23017_init();
	mcp23017_iosetup();

	vTaskDelay(1000 / portTICK_RATE_MS);

	ili_init(spi);
	vTaskDelay(1000 / portTICK_RATE_MS);
	TFT_pushColorRep(0, 0, _width-1, _height-1, TFT_BLACK, _width*_height);

	draw_first_page();

	vTaskDelay(2000 / portTICK_PERIOD_MS);

    if(esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK)
	{
    	printf("efuse OK!");
    	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc1_chan_0_chars);
    	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc1_chan_3_chars);
    	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc1_chan_6_chars);
    	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc1_chan_7_chars);
	}

	continuous_adc_init(adc1_chan_mask, adc2_chan_mask, channel, sizeof(channel) / sizeof(adc_channel_t));

	xTaskCreatePinnedToCore(
			task_read_adc, 			// Function to be called
			"read and convert adc", // Name of task
			2048, 					// Stack size (bytes in ESP32, words in FreeRTOS)
			NULL,					// Parameter to pass to function
			10, 					// Task priority (0 to configMAX_PRIORITIES - 1)
			NULL, 					// Task handle
			0);						// Run on one core for demo purposes (ESP32 only)

	xTaskCreatePinnedToCore(
			task_pzem_request, 		// Function to be called
			"doing PZEM request", 	// Name of task
			4096, 					// Stack size (bytes in ESP32, words in FreeRTOS)
			NULL,					// Parameter to pass to function
			6, 						// Task priority (0 to configMAX_PRIORITIES - 1)
			NULL, 					// Task handle
			0);						// Run on one core for demo purposes (ESP32 only)

	xTaskCreatePinnedToCore(
			task_show_pzem, 		// Function to be called
			"show PZEM", 	// Name of task
			2048, 					// Stack size (bytes in ESP32, words in FreeRTOS)
			NULL,					// Parameter to pass to function
			15, 						// Task priority (0 to configMAX_PRIORITIES - 1)
			NULL, 					// Task handle
			0);		// Run on one core for demo purposes (ESP32 only)

//	xTaskCreatePinnedToCore(
//			task_read_button, 		// Function to be called
//			"read Button", 			// Name of task
//			2048, 					// Stack size (bytes in ESP32, words in FreeRTOS)
//			NULL,					// Parameter to pass to function
//			15, 					// Task priority (0 to configMAX_PRIORITIES - 1)
//			NULL, 					// Task handle
//			0);						// Run on one core for demo purposes (ESP32 only)

//	xTaskCreatePinnedToCore(
//			blink_task, 	// Function to be called
//			"blink LED", 	// Name of task
//			1024, 			// Stack size (bytes in ESP32, words in FreeRTOS)
//			NULL,			// Parameter to pass to function
//			12, 				// Task priority (0 to configMAX_PRIORITIES - 1)
//			NULL, 			// Task handle
//			0);		// Run on one core for demo purposes (ESP32 only)
//	xTaskCreatePinnedToCore(
//			write_uart, 	// Function to be called
//			"SEND UART", 	// Name of task
//			1024, 			// Stack size (bytes in ESP32, words in FreeRTOS)
//			NULL,			// Parameter to pass to function
//			2, 				// Task priority (0 to configMAX_PRIORITIES - 1)
//			NULL, 			// Task handle
//			0);		// Run on one core for demo purposes (ESP32 only)

//	GPS_Init();
//	while(1)
//	{
//		//		vTaskDelay(1000 / portTICK_PERIOD_MS);
//	}

}
