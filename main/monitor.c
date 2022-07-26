/*
 * monitor.c
 *
 *  Created on: May 24, 2022
 *      Author: Lenovo
 */

#include "monitor.h"

void MON_Init(void)
{
	uart_config_t uart_config = {
			.baud_rate 	= 9600,
			.data_bits	= UART_DATA_8_BITS,
			.parity		= UART_PARITY_DISABLE,
			.stop_bits	= UART_STOP_BITS_1,
			.flow_ctrl	= UART_HW_FLOWCTRL_DISABLE
	};

	ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));

	//Set UART pins (using UART0 default pins ie no changes.)
	ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

	//Install UART driver, and get the queue.
	ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0));

	// release the pre registered UART handler/subroutine
	ESP_ERROR_CHECK(uart_isr_free(UART_NUM_0));
}

void MON_WriteStream(uint8_t *buffer, uint8_t length)
{
	uart_write_bytes(UART_NUM_0, (const char*) buffer, length);
}

