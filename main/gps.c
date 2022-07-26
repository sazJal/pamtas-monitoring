/*
 * gps.c
 *
 *  Created on: May 20, 2022
 *      Author: Lenovo
 */


#include "gps.h"

static intr_handle_t handle_console;

static void IRAM_ATTR uart_intr_handle(void *arg)
{
		uint32_t status;
		uint8_t rcvBuff;

		status = UART1.int_st.val;
		rcvBuff = UART1.fifo.rw_byte;
		uart_write_bytes(UART_NUM_0, &rcvBuff, 1);
		uart_clear_intr_status(UART_NUM_1,  UART_RXFIFO_FULL_INT_CLR|UART_RXFIFO_TOUT_INT_CLR);
}

void GPS_Init(void)
{
	uart_config_t uart_config = {
					.baud_rate 	= 9600,
					.data_bits	= UART_DATA_8_BITS,
					.parity		= UART_PARITY_DISABLE,
					.stop_bits	= UART_STOP_BITS_1,
					.flow_ctrl	= UART_HW_FLOWCTRL_DISABLE
			};

			ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));

			//Set UART pins (using UART0 default pins ie no changes.)
			ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, GPIO_NUM_10, GPIO_NUM_9, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

			//Install UART driver, and get the queue.
			ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0));

			// release the pre registered UART handler/subroutine
			ESP_ERROR_CHECK(uart_isr_free(UART_NUM_1));

	//		// register new UART subroutine
			ESP_ERROR_CHECK(uart_isr_register(UART_NUM_1, uart_intr_handle, NULL, ESP_INTR_FLAG_IRAM, &handle_console));
	//
	//		// enable RX interrupt
			ESP_ERROR_CHECK(uart_enable_rx_intr(UART_NUM_1));
}

void GPS_WriteStream(uint8_t *buffer, uint8_t length)
{
	uart_write_bytes(UART_NUM_1, (const char*) buffer, length);
}
