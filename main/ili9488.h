/*
 * ili9488.h
 *
 *  Created on: May 18, 2022
 *      Author: Lenovo
 */

#ifndef MAIN_ILI9488_H_
#define MAIN_ILI9488_H_

#include "tft.h"
#include "tftfunc.h"
#include "driver/gpio.h"
#include "pzem_mux.h"

#define DELAY 0x80
#define SPI_BUS HSPI_HOST
//static const uint8_t ILI9488_init[];

spi_nodma_device_handle_t spi;
spi_nodma_device_handle_t tsspi;

spi_nodma_bus_config_t buscfg;
spi_nodma_device_interface_config_t devcfg;

typedef enum
{
	DISP_FIRST_PAGE_MAIN,
	DISP_FIRST_PAGE_EDIT_SELECT,
	DISP_SECOND_PAGE
} display_state_t;

display_state_t disp_state;

void ili_init(spi_nodma_device_handle_t spi);
void commandList(spi_nodma_device_handle_t spi, const uint8_t *addr) ;

//void show_first_page(pzem_t data, float limit, float sisa, uint32_t v_pv, uint32_t i_pv, uint32_t v_bat, uint32_t i_bat);
//void show_second_page(pzem_t data, uint32_t v_pv, uint32_t i_pv, uint32_t v_bat, uint32_t i_bat);

void show_first_page(pzem_t data, float limit, float sisa, source_param_t source_param);
void show_second_page(pzem_t data, source_param_t source_param);
void draw_first_page(void);
void clean_first_page(void);
void draw_second_page(void);
void clean_second_page(void);
void clear_first_page_variables(void);
void clear_values_first(void);
void clear_values_second(void);
void draw_battery_cover(void);
void draw_battery_point(uint8_t level);

void draw_up_triangle(void);
void draw_down_triangle(void);
void erase_up_triangle(void);
void erase_down_triangle(void);

void draw_edit_box(void);
void erase_edit_box(void);
#endif /* MAIN_ILI9488_H_ */
