/*
 * mcp23017.h
 *
 *  Created on: May 18, 2022
 *      Author: Lenovo
 */

#ifndef MAIN_MCP23017_H_
#define MAIN_MCP23017_H_

#include "driver/i2c.h"
#include "battery.h"

#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define MCP23017_ADDRESS 	0x27
#define AT24C128_ADDRESS	0x51

// registers
#define MCP23017_IODIRA 	0x00
#define MCP23017_IPOLA 		0x02
#define MCP23017_GPINTENA 	0x04
#define MCP23017_DEFVALA 	0x06
#define MCP23017_INTCONA 	0x08
#define MCP23017_IOCONA 	0x0A
#define MCP23017_GPPUA 		0x0C
#define MCP23017_INTFA 		0x0E
#define MCP23017_INTCAPA 	0x10
#define MCP23017_GPIOA 		0x12
#define MCP23017_OLATA 		0x14


#define MCP23017_IODIRB 	0x01
#define MCP23017_IPOLB 		0x03
#define MCP23017_GPINTENB 	0x05
#define MCP23017_DEFVALB 	0x07
#define MCP23017_INTCONB 	0x09
#define MCP23017_IOCONB 	0x0B
#define MCP23017_GPPUB 		0x0D
#define MCP23017_INTFB 		0x0F
#define MCP23017_INTCAPB 	0x11
#define MCP23017_GPIOB 		0x13
#define MCP23017_OLATB 		0x15
#define MCP23017_INT_ERR 	255

/* IO Map */
#define MCP23017_MASK_GPS_PPM		0x01
#define MCP23017_MASK_BUTTON_BACK	0x02
#define MCP23017_MASK_BUTTON_UP		0x04
#define MCP23017_MASK_BUTTON_DOWN	0x08
#define MCP23017_MASK_BUTTON_OK		0x10
#define MCP23017_MASK_BUTTON_EDIT	0x12
/* to be updated to the following later  */
//#define MCP23017_MASK_BUTTON_BACK	0x01
//#define MCP23017_MASK_BUTTON_UP		0x02
//#define MCP23017_MASK_BUTTON_DOWN	0x04
//#define MCP23017_MASK_BUTTON_OK		0x08
//#define MCP23017_MASK_BUTTON_EDIT	0x09

#define MCP23017_POS_GPS_PPM		0
#define MCP23017_POS_BUTTON_BACK	1
#define MCP23017_POS_BUTTON_UP		2
#define MCP23017_POS_BUTTON_DOWN	3
#define MCP23017_POS_BUTTON_OK		4

#define MCP23017_LCD_RESET		0x01
#define MCP23017_GPS_ENABLE		0x02
#define MCP23017_GPS_RESET		0x04
#define MCP23017_BUZZER			0x08
#define MCP23017_RELAY_CTRL_1	0x10
#define MCP23017_RELAY_CTRL_2	0x20

#define MCP23017_PZEM_SEL_1		0x41
#define MCP23017_PZEM_SEL_0		0x81

#define MCP23017_MASK_LCD_RESET		~(0x01)
#define MCP23017_MASK_GPS_ENABLE	~(0x02)
#define MCP23017_MASK_GPS_RESET		~(0x04)
#define MCP23017_MASK_BUZZER		~(0x08)
#define MCP23017_MASK_RELAY_CTRL_1	~(0x10)
#define MCP23017_MASK_RELAY_CTRL_2	~(0x20)
#define MCP23017_MASK_PZEM_SEL_1	~(0x40)
#define MCP23017_MASK_PZEM_SEL_0	~(0x80)

typedef struct
{
	bool isButtonUpPressed;
	bool isButtonDownPressed;
	bool isButtonOkPressed;
	bool isButtonBackPressed;
	bool isButtonEditPressed;
} button_status_t;

button_status_t btn_status;
void at24c128_write(uint16_t regAddr, uint8_t regValue);
void at24c128_4byte_write(uint16_t regAddr, uint32_t regValue);
uint8_t at24c128_read(uint16_t regAddr);
uint32_t at24c128_4byte_read(uint16_t regAddr);
void at24c128_write_record(battery_param_t *param);
esp_err_t at24c128_read_record(battery_param_t *param);

esp_err_t mcp23017_init(void);
void clear_button_status(button_status_t *button);
void mcp23017_writeRegister(uint8_t regAddr, uint8_t regValue);
uint8_t mcp23017_readRegister(uint8_t addr);
void mcp23017_iosetup(void);


#endif /* MAIN_MCP23017_H_ */
