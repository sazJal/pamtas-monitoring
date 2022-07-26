/*
 * pzem_mux.h
 *
 *  Created on: May 24, 2022
 *      Author: Lenovo
 */

#ifndef MAIN_PZEM_MUX_H_
#define MAIN_PZEM_MUX_H_


#include "driver/uart.h"
#include "driver/gpio.h"
#include "soc/uart_struct.h"
#include "soc/uart_reg.h"
#include "stdbool.h"

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

#define	CRC_POLY_16			0xA001
#define	CRC_START_16		0xFFFF

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

#define PZEM_DEFAULT_ADDRESS 0xF8

#define WREG_ALARM_THR   0x0001
#define WREG_ADDR        0x0002

typedef enum
{
	PZEM_CMD_RHR  = 0x03, // Read Holding Register
	PZEM_CMD_RIR  = 0X04, // Read Input Register
	PZEM_CMD_WSR  = 0x06, // Write Single Register
	PZEM_CMD_CAL  = 0x41, // Calibration
	PZEM_CMD_REST = 0x42  // Reset energy
} command_type_t;

typedef enum
{
	PZEM_CHECK_SLAVE_ADDR,
	PZEM_CHECK_COMMAND_TYPE,
	PZEM_CHECK_LENGTH,
	PZEM_CHECK_DATA,
	PZEM_CHECK_CRC16
} receiving_state_t;

typedef enum
{
	PZEM_CHAN_AC_POWER_1,
	PZEM_CHAN_AC_POWER_2,
	PZEM_CHAN_DC_POWER
} channel_select_t;

typedef struct
{
	float voltage;
    float current;
    float power;
    float energy;
    float frequency;
    float pf;
    uint16_t alarms;
    uint8_t slave_address;
    bool  data_is_valid;
} pzem_data_t;

typedef struct
{
	pzem_data_t data[3];
	command_type_t cmd;
	channel_select_t chn;
} pzem_t;

typedef struct
{
	float pv_voltage;
	float pv_current;
	float pv_power;
	float batt_voltage;
	float batt_current;
	float batt_power;
} source_param_t;



void crc16_table_init(void);
uint16_t crc16(const unsigned char *input_str, unsigned char num_bytes);

uint8_t buff[30];
pzem_data_t pzem_data;
pzem_t pzem;
uint8_t pzem_addr;
uint8_t raw_length;
source_param_t source;

void PZEMMux_Init(pzem_t *data);
pzem_data_t PZEMMux_ClearData(void);
pzem_data_t PZEMMux_ParseData(uint8_t *data, uint8_t length);
//esp_err_t PZEMMux_SetSlaveRegister(uint8_t *SlaveReg, uint8_t no_device);
void PZEMMux_WriteStream(uint8_t *buffer, uint8_t length);
void PZEMMux_SendCommand(command_type_t cmd, uint8_t slave_addr, uint16_t reg_addr, uint16_t value);

#endif /* MAIN_PZEM_MUX_H_ */
