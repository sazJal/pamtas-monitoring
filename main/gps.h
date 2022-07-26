/*
 * gps.h
 *
 *  Created on: May 20, 2022
 *      Author: Lenovo
 */

#ifndef MAIN_GPS_H_
#define MAIN_GPS_H_

#include "driver/uart.h"
#include "driver/gpio.h"
#include "soc/uart_struct.h"
#include "soc/uart_reg.h"
#include "stdbool.h"

#define BUF_SIZE (1024)

void GPS_Init(void);
void GPS_WriteStream(uint8_t *buffer, uint8_t length);


#endif /* MAIN_GPS_H_ */
