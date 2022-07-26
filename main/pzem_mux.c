/*
 * pzem_mux.c
 *
 *  Created on: May 24, 2022
 *      Author: Lenovo
 */

#include "pzem_mux.h"
#include "stdbool.h"
#include "stdio.h"
#include "string.h"

static bool crc_tab16_init = false;
static uint16_t  crc_tab16[256];

uint8_t buff[30];
pzem_data_t pzem_data;
pzem_t pzem;
uint8_t pzem_addr;
uint8_t raw_length;
source_param_t source;

static intr_handle_t handle_console;

static void IRAM_ATTR pzem_intr_handle(void *arg)
{
	static receiving_state_t rcv_state = PZEM_CHECK_SLAVE_ADDR;
	static uint8_t length;
	static uint8_t count;
	static uint16_t crcRcv, crcBuff;
	uint8_t rcvBuff;

	char buffData[100];
	uint16_t buff16;
	uint32_t buff32;
	float buffFloat;

	rcvBuff = UART2.fifo.rw_byte;

	switch(rcv_state)
	{
	case PZEM_CHECK_SLAVE_ADDR	:
		if(rcvBuff == pzem_addr)
		{
			pzem_data.slave_address = rcvBuff;
			buff[0] = rcvBuff;
			rcv_state = PZEM_CHECK_COMMAND_TYPE;
		}
		break;
	case PZEM_CHECK_COMMAND_TYPE :
		{
			if(rcvBuff == PZEM_CMD_RIR)
			{
				buff[1] = rcvBuff;
				rcv_state = PZEM_CHECK_LENGTH;
				length = 0;
				count = 0;
			}
		}
		break;
	case PZEM_CHECK_LENGTH :
		length    = rcvBuff;
		raw_length= rcvBuff;
		buff[2]   = rcvBuff;
		rcv_state = PZEM_CHECK_DATA;
		break;
	case PZEM_CHECK_DATA :
		if(count==length-1)
		{
			buff[3+count] = rcvBuff;
			count 		  = 0;
			rcv_state 	  = PZEM_CHECK_CRC16;
		}
		else
		{
			buff[3+count] = rcvBuff;
			count++;
			rcv_state = PZEM_CHECK_DATA;
		}
		break;
	case PZEM_CHECK_CRC16 :
		if(count == 1)
		{
			buff[3+length+count] = rcvBuff;
			crcBuff = crc16((unsigned char *) &buff[0], length+3);
			crcRcv |= (((uint16_t) rcvBuff) << 8);

			if(crcRcv == crcBuff)
			{
				buff16 = ((uint16_t) buff[3]) << 8 | ((uint16_t) buff[4]);
				pzem_data.voltage = buff16*0.1;
				buff32 = (((uint32_t) buff[7]) << 24) |(((uint32_t) buff[8]) << 16) | (((uint32_t) buff[5]) << 8) | (((uint32_t) buff[6]));
				pzem_data.current = buff32*0.001;
				buff32 = (((uint32_t) buff[11]) << 24) |(((uint32_t) buff[12]) << 16) | (((uint32_t) buff[9]) << 8) | (((uint32_t) buff[10]));
				pzem_data.power = buff32*0.1;
				buff16 = ((uint16_t) buff[17]) << 8 | ((uint16_t) buff[18]);
				pzem_data.frequency = buff16*0.1;
				buff32 = (((uint32_t) buff[15]) << 24) |(((uint32_t) buff[16]) << 16) | (((uint32_t) buff[13]) << 8) | (((uint32_t) buff[14]));
				pzem_data.energy = buff32*1.0;
				pzem_data.data_is_valid = true;
			}
			else
			{
				pzem_data = PZEMMux_ClearData();
			}

			rcv_state = PZEM_CHECK_SLAVE_ADDR;
		}
		else
		{
			buff[3+length+count] = rcvBuff;
			crcRcv = (uint16_t) rcvBuff;
			count++;
		}
		break;
	}

//	uart_write_bytes(UART_NUM_0, (char*) &rcvBuff, 1);
//	if(done)
//	{
//		buff16 = ((uint16_t) buff[3]) << 8 | ((uint16_t) buff[4]);
//		buffFloat = buff16*0.1;
//		sprintf(buffData,"Voltage = %f \r\n", buffFloat);
//		uart_write_bytes(UART_NUM_0, (char *)&buffData, strlen(buffData));
//
//		buff32 = (((uint32_t) buff[7]) << 24) |(((uint32_t) buff[8]) << 16) | (((uint32_t) buff[5]) << 8) | (((uint32_t) buff[6]));
//		buffFloat = buff32*0.001;
//		sprintf(buffData,"Current = %f \r\n", buffFloat);
//		uart_write_bytes(UART_NUM_0, (char *)&buffData, strlen(buffData));
//
//		buff32 = (((uint32_t) buff[11]) << 24) |(((uint32_t) buff[12]) << 16) | (((uint32_t) buff[9]) << 8) | (((uint32_t) buff[10]));
//		buffFloat = buff32*0.1;
//		sprintf(buffData,"Power = %f \r\n", buffFloat);
//		uart_write_bytes(UART_NUM_0, (char *)&buffData, strlen(buffData));
//
//		buff16 = ((uint16_t) buff[17]) << 8 | ((uint16_t) buff[18]);
//		buffFloat = buff16*0.1;
//		sprintf(buffData,"Frequency = %f \r\n", buffFloat);
//		uart_write_bytes(UART_NUM_0, (char *)&buffData, strlen(buffData));
//		done = false;
//	}
//	uart_flush(UART_NUM_2);
	uart_clear_intr_status(UART_NUM_2,  UART_RXFIFO_FULL_INT_CLR|UART_RXFIFO_TOUT_INT_CLR);
//	uart_clear_intr_status(UART_NUM_2,  status);
}

void PZEMMux_Init(pzem_t *data)
{
	uart_config_t uart_config = {
			.baud_rate 	= 9600,
			.data_bits	= UART_DATA_8_BITS,
			.parity		= UART_PARITY_DISABLE,
			.stop_bits	= UART_STOP_BITS_1,
			.flow_ctrl	= UART_HW_FLOWCTRL_DISABLE
	};

	data->chn = PZEM_CHAN_AC_POWER_1;
	data->cmd = PZEM_CMD_RHR;

	ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));

	//Set UART pins (using UART0 default pins ie no changes.)
	ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, GPIO_NUM_17, GPIO_NUM_16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

	//Install UART driver, and get the queue.
	ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, BUF_SIZE*2, 0, 0, NULL, 0));

	// release the pre registered UART handler/subroutine
//	ESP_ERROR_CHECK(uart_isr_free(UART_NUM_2));
//
////	// register new UART subroutine
//	ESP_ERROR_CHECK(uart_isr_register(UART_NUM_2, pzem_intr_handle, NULL, ESP_INTR_FLAG_IRAM, &handle_console));
////
////	// enable RX interrupt
//	ESP_ERROR_CHECK(uart_enable_rx_intr(UART_NUM_2));
}

//esp_err_t PZEMMux_SetSlaveRegister(uint8_t SlaveReg, uint8_t no_device)
//{
//	esp_err_t status;
//
//	PZEMMux_SendCommand(PZEM_CMD_WSR, PZEM_DEFAULT_ADDRESS, WREG_ADDR, SlaveReg);
//	return status;
//}

pzem_data_t PZEMMux_ClearData(void)
{
	pzem_data_t data;
	data.voltage = 0.0;
	data.current = 0.0;
	data.energy = 0.0;
	data.frequency = 0.0;
	data.pf = 0.0;
	data.power = 0.0;
	data.data_is_valid = false;
	data.slave_address = 0xF8;
	return data;
}
pzem_data_t PZEMMux_ParseData(uint8_t *data, uint8_t length)
{
	receiving_state_t rcv_state = PZEM_CHECK_SLAVE_ADDR;
	uint8_t length_data;
	uint8_t count;
	uint16_t crcRcv, crcBuff;

	uint16_t buff16;
	uint32_t buff32;

	pzem_data_t pzem_data;

	for(uint8_t i=0; i<length; i++)
	{
		switch(rcv_state)
		{
		case PZEM_CHECK_SLAVE_ADDR	:
			if(data[i] == pzem_addr)
			{
				pzem_data.slave_address = data[i];
				buff[0] = data[i];
				rcv_state = PZEM_CHECK_COMMAND_TYPE;
			}
			break;
		case PZEM_CHECK_COMMAND_TYPE :
			{
				if(data[i] == PZEM_CMD_RIR)
				{
					buff[1] = data[i];
					rcv_state = PZEM_CHECK_LENGTH;
					length_data = 0;
					count = 0;
				}
			}
			break;
		case PZEM_CHECK_LENGTH :
			length_data    = data[i];
			buff[2]   = data[i];
			rcv_state = PZEM_CHECK_DATA;
			break;
		case PZEM_CHECK_DATA :
			if(count==length_data-1)
			{
				buff[3+count] = data[i];
				count 		  = 0;
				rcv_state 	  = PZEM_CHECK_CRC16;
			}
			else
			{
				buff[3+count] = data[i];
				count++;
				rcv_state = PZEM_CHECK_DATA;
			}
			break;
		case PZEM_CHECK_CRC16 :
			if(count == 1)
			{
				buff[3+length_data+count] = data[i];
				crcBuff = crc16((unsigned char *) &buff[0], length_data+3);
				crcRcv |= (((uint16_t) data[i]) << 8);

				if(crcRcv == crcBuff)
				{
					pzem_data.data_is_valid = true;
					buff16 = ((uint16_t) buff[3]) << 8 | ((uint16_t) buff[4]);
					pzem_data.voltage = buff16*0.1;
					buff32 = (((uint32_t) buff[7]) << 24) |(((uint32_t) buff[8]) << 16) | (((uint32_t) buff[5]) << 8) | (((uint32_t) buff[6]));
					pzem_data.current = buff32*0.001;
					buff32 = (((uint32_t) buff[11]) << 24) |(((uint32_t) buff[12]) << 16) | (((uint32_t) buff[9]) << 8) | (((uint32_t) buff[10]));
					pzem_data.power = buff32*0.1;
					buff16 = ((uint16_t) buff[17]) << 8 | ((uint16_t) buff[18]);
					pzem_data.frequency = buff16*0.1;
					buff32 = (((uint32_t) buff[15]) << 24) |(((uint32_t) buff[16]) << 16) | (((uint32_t) buff[13]) << 8) | (((uint32_t) buff[14]));
					pzem_data.energy = buff32*1.0;
					pzem_data.data_is_valid = true;
				}
				rcv_state = PZEM_CHECK_SLAVE_ADDR;
			}
			else
			{
				buff[3+length_data+count] = data[i];
				crcRcv = (uint16_t) data[i];
				count++;
			}
			break;
		}
	}

	return pzem_data;
}
void PZEMMux_WriteStream(uint8_t *buffer, uint8_t length)
{
	uart_write_bytes(UART_NUM_2, (const char*) buffer, length);
}

void PZEMMux_SendCommand(command_type_t cmd, uint8_t slave_addr, uint16_t reg_addr, uint16_t value)
{
	static uint8_t sendBuffer[8]; // Send buffer
	uint16_t crc;
	uint8_t length;
	length = 0;
	switch(cmd)
	{
		case PZEM_CMD_RHR	:
			length = 6;
			sendBuffer[0] = slave_addr;               // Set slave address
			sendBuffer[1] = 0x03;                     // Set command
			sendBuffer[2] = (reg_addr >> 8) & 0xFF;   // Register Address High Byte
			sendBuffer[3] = (reg_addr) & 0xFF;        // Register Address Low Byte
			sendBuffer[4] = (value >> 8) & 0xFF;      // Number of Registers High Byte
			sendBuffer[5] = (value) & 0xFF;           // Number of Registers Low Byte
			crc = crc16((unsigned char *) &sendBuffer[0], length);
			sendBuffer[7] = (crc >> 8) & 0xFF; 		  // CRC16 High Byte second
			sendBuffer[6] = crc & 0xFF; 			  // CRC16 Low byte first
			break;
		case PZEM_CMD_RIR	:
			length = 6;
			sendBuffer[0] = slave_addr;               // Set slave address
			sendBuffer[1] = 0x04;                     // Set command
			sendBuffer[2] = (reg_addr >> 8) & 0xFF;   // Register Address High Byte
			sendBuffer[3] = (reg_addr) & 0xFF;        // Register Address Low Byte
			sendBuffer[4] = (value >> 8) & 0xFF;      // Number of Registers High Byte
			sendBuffer[5] = (value) & 0xFF;           // Number of Registers Low Byte
			crc = crc16((unsigned char *) &sendBuffer[0], length);
			sendBuffer[7] = (crc >> 8) & 0xFF; 		  // CRC16 High Byte second
			sendBuffer[6] = crc & 0xFF; 			  // CRC16 Low byte first
			break;
		case PZEM_CMD_WSR	:
			length = 6;
			sendBuffer[0] = slave_addr;              // Set slave address
			sendBuffer[1] = 0x06;                     // Set command
			sendBuffer[2] = (reg_addr >> 8) & 0xFF;   // Register Address High Byte
			sendBuffer[3] = (reg_addr) & 0xFF;         // Register Address Low Byte
			sendBuffer[4] = (value >> 8) & 0xFF;       // Register Value High Byte
			sendBuffer[5] = (value) & 0xFF;            // Register Value Low Byte
			crc = crc16((unsigned char *) &sendBuffer[0], length);
			sendBuffer[7] = (crc >> 8) & 0xFF; 		  // CRC16 High Byte second
			sendBuffer[6] = crc & 0xFF; 			  // CRC16 Low byte first
			break;
		case PZEM_CMD_CAL	:
			length = 4;
			sendBuffer[0] = 0xF8;   // Preserved
			sendBuffer[1] = 0x41;   // Preserved
			sendBuffer[2] = 0x37;   // Preserved
			sendBuffer[3] = 0x21;   // Preserved
			crc = crc16((unsigned char *) &sendBuffer[0], length);
			sendBuffer[5] = (crc >> 8) & 0xFF; 		  // CRC16 High Byte second
			sendBuffer[4] = crc & 0xFF; 			  // CRC16 Low byte first
			break;
		case PZEM_CMD_REST	:
			length = 2;
			sendBuffer[0] = slave_addr;              // Set slave address
			sendBuffer[1] = 0x42;                     // Set command
			crc = crc16((unsigned char *) &sendBuffer[0], length);
			sendBuffer[3] = (crc >> 8) & 0xFF; 		  // CRC16 High Byte second
			sendBuffer[2] = crc & 0xFF; 			  // CRC16 Low byte first
			break;
	}
	PZEMMux_WriteStream((uint8_t*) &sendBuffer[0], length+2);
}

void crc16_table_init(void)
{
	uint16_t i;
	uint16_t j;
	uint16_t crc;
	uint16_t c;

	for (i=0; i<256; i++) {

		crc = 0;
		c   = i;

		for (j=0; j<8; j++) {

			if ( (crc ^ c) & 0x0001 ) crc = ( crc >> 1 ) ^ CRC_POLY_16;
			else                      crc =   crc >> 1;

			c = c >> 1;
		}

		crc_tab16[i] = crc;
	}

	crc_tab16_init = true;
}

uint16_t crc16(const unsigned char *input_str, unsigned char num_bytes)
{
	uint16_t crc;
	uint16_t tmp;
	uint16_t short_c;
	const unsigned char *ptr;
	unsigned char a;

	if (!crc_tab16_init) crc16_table_init();

	crc = CRC_START_16;
	ptr = input_str;

	if ( ptr != NULL ) for (a=0; a<num_bytes; a++) {

		short_c = 0x00ff & (uint16_t) *ptr;
		tmp     =  crc       ^ short_c;
		crc     = (crc >> 8) ^ crc_tab16[ tmp & 0xff ];

		ptr++;
	}

	return crc;
}
