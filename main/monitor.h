/*
 * monitor.h
 *
 *  Created on: May 24, 2022
 *      Author: Lenovo
 */

#ifndef MAIN_MONITOR_H_
#define MAIN_MONITOR_H_

#include "driver/uart.h"
#include "driver/gpio.h"
#include "soc/uart_struct.h"
#include "soc/uart_reg.h"

#define BUF_SIZE (1024)

void MON_Init(void);
void MON_WriteStream(uint8_t *buffer, uint8_t length);

#endif /* MAIN_MONITOR_H_ */
