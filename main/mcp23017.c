/*
 * mcp23017.c
 *
 *  Created on: May 18, 2022
 *      Author: Lenovo
 */

#include "mcp23017.h"
#include "stdbool.h"

button_status_t btn_status;

void clear_button_status(button_status_t *button)
{
	button->isButtonBackPressed		= false;
	button->isButtonUpPressed		= false;
	button->isButtonOkPressed		= false;
	button->isButtonDownPressed		= false;
	button->isButtonEditPressed		= false;
}

void at24c128_write(uint16_t regAddr, uint8_t regValue)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (AT24C128_ADDRESS << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, (uint8_t)(regAddr>>8), 	true);
	i2c_master_write_byte(cmd, (uint8_t)(regAddr&0xFF), true);
	i2c_master_write_byte(cmd, regValue, true);
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 0);
	i2c_cmd_link_delete(cmd);
}

void at24c128_4byte_write(uint16_t regAddr, uint32_t regValue)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (AT24C128_ADDRESS << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, (uint8_t)(regAddr>>8), 	true);
	i2c_master_write_byte(cmd, (uint8_t)(regAddr&0xFF), true);
	i2c_master_write_byte(cmd, (regValue >> 24) & 0xFF, true);
	i2c_master_write_byte(cmd, (regValue >> 16) & 0xFF, true);
	i2c_master_write_byte(cmd, (regValue >> 8) & 0xFF, true);
	i2c_master_write_byte(cmd, regValue & 0xFF, true);
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 0);
	i2c_cmd_link_delete(cmd);
}

uint8_t at24c128_read(uint16_t regAddr)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (AT24C128_ADDRESS << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd, (uint8_t)(regAddr>>8), 	true);
	i2c_master_write_byte(cmd, (uint8_t)(regAddr&0xFF), true);
	i2c_master_stop(cmd);

	i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (AT24C128_ADDRESS << 1) | I2C_MASTER_READ, 1);
	uint8_t tmpByte;
	i2c_master_read_byte(cmd, &tmpByte, I2C_MASTER_NACK);
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 0);
	i2c_cmd_link_delete(cmd);

	return tmpByte;
}

uint32_t at24c128_4byte_read(uint16_t regAddr)
{
	uint32_t data;
	uint8_t tmpByte0, tmpByte1, tmpByte2, tmpByte3;
	data = 0;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (AT24C128_ADDRESS << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd, (uint8_t)(regAddr>>8), 	true);
	i2c_master_write_byte(cmd, (uint8_t)(regAddr&0xFF), true);
	i2c_master_stop(cmd);

	i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (AT24C128_ADDRESS << 1) | I2C_MASTER_READ, 1);
	i2c_master_read_byte(cmd, &tmpByte0, I2C_MASTER_ACK);
	i2c_master_read_byte(cmd, &tmpByte1, I2C_MASTER_ACK);
	i2c_master_read_byte(cmd, &tmpByte2, I2C_MASTER_ACK);
	i2c_master_read_byte(cmd, &tmpByte3, I2C_MASTER_NACK);

	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 0);
	i2c_cmd_link_delete(cmd);

	data = tmpByte0;
	data = (data << 8) | tmpByte1;
	data = (data << 8) | tmpByte2;
	data = (data << 8) | tmpByte3;
	return data;
}

void mcp23017_writeRegister(uint8_t regAddr, uint8_t regValue)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (MCP23017_ADDRESS << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, regAddr, true);
	i2c_master_write_byte(cmd, regValue, true);
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 0);
	i2c_cmd_link_delete(cmd);
}

uint8_t mcp23017_readRegister(uint8_t addr)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (MCP23017_ADDRESS << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd, addr, 1);
	i2c_master_stop(cmd);

	i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (MCP23017_ADDRESS << 1) | I2C_MASTER_READ, 1);
	uint8_t tmpByte;
	i2c_master_read_byte(cmd, &tmpByte, I2C_MASTER_NACK);
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 0);
	i2c_cmd_link_delete(cmd);

	return tmpByte;
}

void at24c128_write_record(battery_param_t *param)
{
	/* write ID */
	at24c128_4byte_write(0x0000, 0x11223344);
	/* write SoC */
	at24c128_4byte_write(0x0004, BAT_FloatToHex(param->SoC));
	/* write SoH */
	at24c128_4byte_write(0x0008, BAT_FloatToHex(param->SoH));
	/* write cycle count */
	at24c128_4byte_write(0x000C, (uint32_t)(param->cycle_count));
	/* write cycle mem */
	at24c128_4byte_write(0x0010, BAT_FloatToHex(param->cycle_mem));
	/* write capacity */
	at24c128_4byte_write(0x0014, BAT_FloatToHex(param->capacity));
	/* write status */
	at24c128_4byte_write(0x0004, (param->status));
}

esp_err_t at24c128_read_record(battery_param_t *param)
{
	return true;
}



esp_err_t mcp23017_init(void)
{
	int i2c_master_port = I2C_NUM_0;

	i2c_config_t config =
	{
			.mode	= I2C_MODE_MASTER,
			.sda_io_num	= GPIO_NUM_32,
			.scl_io_num = GPIO_NUM_33,
			.master.clk_speed = 100000
	};

	i2c_param_config(i2c_master_port, &config);

	return i2c_driver_install(i2c_master_port, config.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

void mcp23017_iosetup(void)
{
	mcp23017_writeRegister(MCP23017_IODIRA, 0x1F);	// set GPIOA0, GPIOA1, GPIOA2, GPIOA3, & GPIOA4 as input.
	mcp23017_writeRegister(MCP23017_IODIRB, 0x00);  // set GPIOB0, GPIOB1, GPIOB2, GPIOB3, GPIOB4, & GPIOB5 as output.
	mcp23017_writeRegister(MCP23017_GPIOB, 0x01);	// turn off LCD reset
}

//uint16_t readRegisterGPIO(uint8_t addr)
//{
//	uint16_t dataWord;
//	uint8_t significant_byte;
//	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
//	i2c_master_start(cmd);
//	i2c_master_write_byte(cmd, (MCP23017_ADDRESS << 1) | I2C_MASTER_WRITE, 1);
//	i2c_master_write_byte(cmd, MCP23017_GPIOA, 1);
//	i2c_master_stop(cmd);
//	i2c_master_cmd_begin(I2C_NUM_0, cmd, 0);
//	i2c_cmd_link_delete(cmd);
//
//	cmd = i2c_cmd_link_create();
//	i2c_master_start(cmd);
//	i2c_master_write_byte(cmd, (MCP23017_ADDRESS << 1) | I2C_MASTER_READ, 1);
//	uint8_t tmpByte;
//	i2c_master_read_byte(cmd, &tmpByte, I2C_MASTER_NACK);
//	i2c_master_stop(cmd);
//	i2c_master_cmd_begin(I2C_NUM_0, cmd, 0);
//	i2c_cmd_link_delete(cmd);
//
//	return tmpByte;
//}


//
//void mcp23017_set_gpioB_output(uint8_t port)
//{
//	writeRegister(MCP23017_GPIOB, port);
//}

//esp_err_t mcp23017_register_write_byte(uint8_t reg_addr, uint8_t data)
//{
//	int ret;
//	uint8_t write_buf[2] = {reg_addr, data};
//
//	ret = i2c_master_write_to_device(I2C_NUM_0, MCP23017_ADDRESS, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
//
//	return ret;
//}
//
//esp_err_t mcp23017_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
//{
//	return i2c_master_write_read_device(I2C_NUM_0, MCP23017_ADDRESS, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
//}
