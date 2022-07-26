/*
 * pzem.h
 *
 *  Created on: May 18, 2022
 *      Author: Lenovo
 */

#ifndef MAIN_PZEM_H_
#define MAIN_PZEM_H_


#include "driver/uart.h"
#include "driver/gpio.h"
#include "soc/uart_struct.h"

//#define	CRC_POLY_16			0xA001
//#define	CRC_START_16		0x0000
//
//#define BUF_SIZE (1024)
//#define RD_BUF_SIZE (BUF_SIZE)
//
//typedef enum
//{
//	PZEM_CMD_RHR  = 0x03, // Read Holding Register
//	PZEM_CMD_RIR  = 0X04, // Read Input Register
//	PZEM_CMD_WSR  = 0x06, // Write Single Register
//	PZEM_CMD_CAL  = 0x41, // Calibration
//	PZEM_CMD_REST = 0x42  // Reset energy
//} command_type_t;
//
//typedef enum
//{
//	PZEM_CHECK_SLAVE_ADDR,
//	PZEM_CHECK_COMMAND_TYPE,
//	PZEM_CHECK_LENGTH,
//	PZEM_CHECK_DATA,
//	PZEM_CHECK_CRC16
//} receiving_state_t;
//
//typedef enum
//{
//	PZEM_CHAN_AC_POWER_1,
//	PZEM_CHAN_AC_POWER_2,
//	PZEM_CHAN_DC_POWER,
//	PZEM_CHAN_GPS
//} channel_select_t;
//
//typedef struct
//{
//	float voltage;
//    float current;
//    float power;
//    float energy;
//    float frequency;
//    float pf;
//    uint16_t alarms;
//    uint8_t slave_address;
//    bool  data_is_valid;
//} pzem_data_t;
//
//typedef struct
//{
//	pzem_data_t data[3];
//	command_type_t cmd;
//	channel_select_t chn;
//} pzem_t;
//
//void crc16_table_init(void);
//uint16_t crc16(const unsigned char *input_str, unsigned char num_bytes);
//
//void PZEM_Init(pzem_t data);
//void PZEM_SelectChannel(channel_select_t channel);
//void PZEM_WriteStream(uint8_t *buffer, uint8_t length);
//void PZEM_SendCommand(command_type_t cmd, uint8_t slave_addr, uint16_t reg_addr, uint16_t value);



#endif /* MAIN_PZEM_H_ */
