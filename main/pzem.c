/*
 * pzem.c
 *
 *  Created on: May 18, 2022
 *      Author: Lenovo
 */

#include "pzem.h"
//#include "stdbool.h"
//
//static bool crc_tab16_init = false;
//static uint16_t  crc_tab16[256];
//void crc16_table_init(void)
//{
//	uint16_t i;
//	uint16_t j;
//	uint16_t crc;
//	uint16_t c;
//
//	for (i=0; i<256; i++) {
//
//		crc = 0;
//		c   = i;
//
//		for (j=0; j<8; j++) {
//
//			if ( (crc ^ c) & 0x0001 ) crc = ( crc >> 1 ) ^ CRC_POLY_16;
//			else                      crc =   crc >> 1;
//
//			c = c >> 1;
//		}
//
//		crc_tab16[i] = crc;
//	}
//
//	crc_tab16_init = true;
//}
//
//uint16_t crc16(const unsigned char *input_str, unsigned char num_bytes)
//{
//	uint16_t crc;
//	uint16_t tmp;
//	uint16_t short_c;
//	const unsigned char *ptr;
//	unsigned char a;
//
//	if (!crc_tab16_init) crc16_table_init();
//
//	crc = CRC_START_16;
//	ptr = input_str;
//
//	if ( ptr != NULL ) for (a=0; a<num_bytes; a++) {
//
//		short_c = 0x00ff & (uint16_t) *ptr;
//		tmp     =  crc       ^ short_c;
//		crc     = (crc >> 8) ^ crc_tab16[ tmp & 0xff ];
//
//		ptr++;
//	}
//
//	return crc;
//}
//
//void PZEM_Init(pzem_t data)
//{
//	uart_config_t uart_config = {
//			.baud_rate 	= 9600,
//			.data_bits	= UART_DATA_8_BITS,
//			.parity		= UART_PARITY_DISABLE,
//			.stop_bits	= UART_STOP_BITS_1,
//			.flow_ctrl	= UART_HW_FLOWCTRL_DISABLE
//	};
//
//	ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
//
//	//Set UART pins (using UART0 default pins ie no changes.)
//	ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
//
//	//Install UART driver, and get the queue.
//	ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0));
//
//	// release the pre registered UART handler/subroutine
//	ESP_ERROR_CHECK(uart_isr_free(UART_NUM_0));
//
//	// register new UART subroutine
////	ESP_ERROR_CHECK(uart_isr_register(UART_NUM_0, uart_intr_handle, &data, ESP_INTR_FLAG_IRAM, NULL));
//
//	// enable RX interrupt
////	ESP_ERROR_CHECK(uart_enable_rx_intr(UART_NUM_0));
//}
//
//void PZEM_WriteStream(uint8_t *buffer, uint8_t length)
//{
//	uart_write_bytes(UART_NUM_0, (const char*) buffer, length);
//}
//
//void PZEM_SendCommand(command_type_t cmd, uint8_t slave_addr, uint16_t reg_addr, uint16_t value)
//{
//}
//
//
//static void IRAM_ATTR uart_intr_handle(void *arg)
//{
////	uint16_t rx_fifo_len;
////	uint32_t status;
////	uint8_t rcvBuff;
////	static uint8_t chn;
////	static uint8_t length;
////	static command_type_t cmd;
////	pzem_t pzem_entity;
////	static uint8_t count=0;
////	static receiving_state_t rcv_state = PZEM_CHECK_SLAVE_ADDR;
////	static uint8_t data_buffer[30];
////	static uint8_t data_crc[30];
////	static uint16_t crc;
////	uint16_t crcBuff;
////
////	pzem_entity = *(pzem_t *)arg;
////	status = UART0.int_st.val;
////	rcvBuff = UART0.fifo.rw_byte;
////	switch(rcv_state)
////	{
////		case PZEM_CHECK_SLAVE_ADDR	:
////			if(rcvBuff == pzem_entity.data[0].slave_address)
////			{
////				data_crc[0] = rcvBuff;
////				chn = 0;
////				rcv_state = PZEM_CHECK_COMMAND_TYPE;
////			}
////			else if(rcvBuff == pzem_entity.data[1].slave_address)
////			{
////				data_crc[0] = rcvBuff;
////				chn = 1;
////				rcv_state = PZEM_CHECK_COMMAND_TYPE;
////			}
////			else if(rcvBuff == pzem_entity.data[2].slave_address)
////			{
////				data_crc[0] = rcvBuff;
////				chn = 2;
////				rcv_state = PZEM_CHECK_COMMAND_TYPE;
////			}
////			break;
////		case PZEM_CHECK_COMMAND_TYPE	:
////			cmd = rcvBuff;
////			data_crc[1] = rcvBuff;
////			length = 0;
////			if(rcvBuff == PZEM_CMD_RIR)
////			{
////				rcv_state = PZEM_CHECK_LENGTH;
////			}
////			else if(rcvBuff == PZEM_CMD_REST)
////			{
////				rcv_state = PZEM_CHECK_CRC16;
////			}
////			break;
////		case PZEM_CHECK_LENGTH		:
////			data_crc[2] = rcvBuff;
////			length = rcvBuff;
////			rcvBuff = PZEM_CHECK_DATA;
////			break;
////		case PZEM_CHECK_DATA			:
////			if(count== length-1)
////			{
////				count = 0;
////				data_crc[count+3]  = rcvBuff;
////				data_buffer[count] = rcvBuff;
////				rcv_state = PZEM_CHECK_CRC16;
////			}
////			else
////			{
////				count++;
////				data_crc[count+3]  = rcvBuff;
////				data_buffer[count] = rcvBuff;
////			}
////			break;
////		case PZEM_CHECK_CRC16		:
////			if(count==1)
////			{
////				count = 0;
////				crc = (crc << 8) | rcvBuff;
////				crcBuff = crc16((unsigned char *) &data_crc[0], length+2);
////				if(crcBuff == crc)
////				{
////					if(cmd == PZEM_CMD_RIR)
////					{
////						pzem_entity.data[chn].voltage = ((uint16_t)data_buffer[0] << 8 | // Raw voltage in 0.1V
////					                              	  	 (uint16_t)data_buffer[1])/10.0;
////
////						pzem_entity.data[chn].current = ((uint32_t)data_buffer[2] << 8 | // Raw current in 0.001A
////					                              	  	 (uint32_t)data_buffer[3] |
////														 (uint32_t)data_buffer[4] << 24 |
////														 (uint32_t)data_buffer[5] << 16) / 1000.0;
////
////						pzem_entity.data[chn].power =   ((uint32_t)data_buffer[6] << 8 | // Raw power in 0.1W
////					                              	  	 (uint32_t)data_buffer[7] |
////														 (uint32_t)data_buffer[8] << 24 |
////														 (uint32_t)data_buffer[9] << 16) / 10.0;
////
////						pzem_entity.data[chn].energy =  ((uint32_t)data_buffer[10] << 8 | // Raw Energy in 1Wh
////					                              	  	 (uint32_t)data_buffer[11] |
////														 (uint32_t)data_buffer[12] << 24 |
////														 (uint32_t)data_buffer[13] << 16) / 1000.0;
////
////						pzem_entity.data[chn].frequency=((uint32_t)data_buffer[14] << 8 | // Raw Frequency in 0.1Hz
////					                              	  	 (uint32_t)data_buffer[15]) / 10.0;
////
////						pzem_entity.data[chn].pf 	=   ((uint16_t)data_buffer[16] << 8 | // Raw pf in 0.01
////					                              	  	 (uint16_t)data_buffer[17])/100.0;
////
////						pzem_entity.data[chn].alarms =  ((uint16_t)data_buffer[18] << 8 | // Raw alarm value
////					                              	  	 (uint16_t)data_buffer[19]);
////						pzem_entity.data[chn].data_is_valid = true;
////					}
////					else if(cmd == PZEM_CMD_REST)
////					{
////						// tells restart success
////					}
////				}
////				rcv_state = PZEM_CHECK_SLAVE_ADDR;
////			}
////			else
////			{
////				crc = rcvBuff;
////				count++;
////			}
////			break;
////	}
//
////	rx_fifo_len = UART0.status.rxfifo_cnt;
////	while(rx_fifo_len)
////	{
////		rxbuff[i++] = UART0.fifo.rw_byte;
////		rx_fifo_len--;
////	}
//
////	uart_clear_intr_status(UART_NUM_0, status);
////	uart_clear_intr_status(EX_UART_NUM, UART_INTR_RXFIFO_FULL|UART_INTR_RXFIFO_TOUT);
////	uart_write_bytes(EX_UART_NUM, (const char*) "RX Done", 7);
////	switch(state)
////	{
////	case RCV_SLAVE_ADDR	:
////		if(rcvBuff == )
////		break;
////	}
////	switch(*(pezm_ *)arg)
////	{
////	case AC_POWER_1	:
////		break;
////	case AC_POWER_2 :
////		break;
////	case DC_POWER	:
////		break;
////	case GPS :
////		break;
////	}
//}
