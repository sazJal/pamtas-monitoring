/*
 * spi_master_nodma.h
 *
 *  Created on: May 19, 2022
 *      Author: Lenovo
 */

#ifndef MAIN_SPI_MASTER_NODMA_H_
#define MAIN_SPI_MASTER_NODMA_H_

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "soc/spi_struct.h"

#include "esp_intr.h"
#include "esp_intr_alloc.h"
#include "rom/lldesc.h"

typedef enum {
    SPI_HOST=0,                     ///< SPI1, SPI; Cannot be used in this driver!
    HSPI_HOST=1,                    ///< SPI2, HSPI
    VSPI_HOST=2                     ///< SPI3, VSPI
} spi_nodma_host_device_t;

typedef struct {
    int mosi_io_num;                ///< GPIO pin for Master Out Slave In (=spi_d) signal, or -1 if not used.
    int miso_io_num;                ///< GPIO pin for Master In Slave Out (=spi_q) signal, or -1 if not used.
    int sclk_io_num;                ///< GPIO pin for Spi CLocK signal, or -1 if not used.
    int quadwp_io_num;              ///< GPIO pin for WP (Write Protect) signal which is used as D2 in 4-bit communication modes, or -1 if not used.
    int quadhd_io_num;              ///< GPIO pin for HD (HolD) signal which is used as D3 in 4-bit communication modes, or -1 if not used.
} spi_nodma_bus_config_t;

#define SPI_DEVICE_TXBIT_LSBFIRST          (1<<0)  ///< Transmit command/address/data LSB first instead of the default MSB first
#define SPI_DEVICE_RXBIT_LSBFIRST          (1<<1)  ///< Receive data LSB first instead of the default MSB first
#define SPI_DEVICE_BIT_LSBFIRST            (SPI_TXBIT_LSBFIRST|SPI_RXBIT_LSBFIRST); ///< Transmit and receive LSB first
#define SPI_DEVICE_3WIRE                   (1<<2)  ///< Use spiq for both sending and receiving data
#define SPI_DEVICE_POSITIVE_CS             (1<<3)  ///< Make CS positive during a transaction instead of negative
#define SPI_DEVICE_HALFDUPLEX              (1<<4)  ///< Transmit data before receiving it, instead of simultaneously
#define SPI_DEVICE_CLK_AS_CS               (1<<5)  ///< Output clock on CS line if CS is active

#define SPI_ERR_OTHER_CONFIG 7001

typedef struct spi_nodma_transaction_t spi_nodma_transaction_t;
typedef void(*transaction_cb_t)(spi_nodma_transaction_t *trans);


typedef struct
{
	uint8_t command_bits;
	uint8_t address_bits;
	uint8_t dummy_bits;
	uint8_t mode;
	uint8_t duty_cycle_pos;
	uint8_t cs_ena_pretrans;
	uint8_t cs_ena_posttrans;
	int clock_speed_hz;
	int spics_io_num;
	int spics_ext_io_num;
	uint32_t flags;
	int queue_size;
	transaction_cb_t pre_cb;
	transaction_cb_t post_cb;
	uint8_t selected;
} spi_nodma_device_interface_config_t;

#define SPI_TRANS_MODE_DIO			(1<<0)	// transmit/receive data in 2-bit mode
#define SPI_TRANS_MODE_QIO			(1<<1)	// transmit/receive data in 4-bit mode
#define SPI_TRANS_MODE_DIOQIO_ADDR	(1<<2)  // also transmit address in mode selected by SPI_MODE_DIO/SPI_MODE_QIO
#define SPI_TRANS_USE_RXDATA		(1<<3)
#define SPI_TRANS_USE_TXDATA		(1<<4)

struct spi_nodma_transaction_t
{
	uint32_t flags;
	uint16_t command;
	uint64_t address;
	size_t length;
	size_t rxlength;
	void *user;
	union
	{
		const void *tx_buffer;
		uint8_t tx_data[4];
	};
	union
	{
		void *rx_buffer;
		uint8_t rx_data[4];
	};
};

#define NO_CS 3
#define NO_DEV 6
#define SPI_SEMAPHORE_WAIT 2000

typedef struct spi_nodma_device_t spi_nodma_device_t;
typedef struct
{
	spi_nodma_device_t *device[NO_DEV];
	intr_handle_t intr;
	spi_dev_t *hw;
	spi_nodma_transaction_t *cur_trans;
	int cur_device;
	lldesc_t dmadesc_tx, dmadesc_rx;
	bool no_gpio_matrix;
	QueueHandle_t spi_nodma_bus_mutex;
	spi_nodma_bus_config_t cur_bus_config;
} spi_nodma_host_t;

struct spi_nodma_device_t
{
	QueueHandle_t trans_queue;
	QueueHandle_t ret_queue;
	spi_nodma_device_interface_config_t cfg;
	spi_nodma_host_t *host;
	spi_nodma_bus_config_t bus_config;
	spi_nodma_host_device_t host_dev;
};

typedef struct spi_nodma_device_t* 					spi_nodma_device_handle_t;
typedef struct spi_nodma_host_t*					spi_nodma_host_handle_t;
typedef struct spi_nodma_device_interface_config_t* spi_nodma_device_interface_config_handle_t;

// the explanation of following fuctions can be found in https://github.com/loboris/ESP32_SPI_MASTER_NODMA_EXAMPLE/blob/master/components/tft/spi_master_nodma.h

esp_err_t spi_nodma_bus_add_device(spi_nodma_host_device_t host, spi_nodma_bus_config_t *bus_config, spi_nodma_device_interface_config_t *dev_config, spi_nodma_device_handle_t *handle);
esp_err_t spi_nodma_bus_remove_device(spi_nodma_device_handle_t handle);
uint32_t spi_nodma_get_speed(spi_nodma_device_handle_t handle);
uint32_t spi_nodma_set_speed(spi_nodma_device_handle_t handle, uint32_t speed);
esp_err_t spi_nodma_device_select(spi_nodma_device_handle_t handle, int force);
esp_err_t spi_nodma_device_deselect(spi_nodma_device_handle_t handle);
bool spi_nodma_uses_native_pins(spi_nodma_device_handle_t handle);
void spi_nodma_get_native_pins(int host, int *sdi, int *sdo, int *sck);
esp_err_t spi_nodma_transfer_data(spi_nodma_device_handle_t handle, spi_nodma_transaction_t *trans);
esp_err_t spi_device_queue_trans(spi_nodma_device_handle_t handle, spi_nodma_transaction_t *trans_desc, TickType_t ticks_to_wait);
esp_err_t spi_device_get_trans_result(spi_nodma_device_handle_t handle, spi_nodma_transaction_t **trans_desc, TickType_t ticks_to_wait);
esp_err_t spi_device_transmit(spi_nodma_device_handle_t handle, spi_nodma_transaction_t *trans_desc);
esp_err_t spi_nodma_device_TakeSemaphore(spi_nodma_device_handle_t handle);
void spi_nodma_device_GiveSemaphore(spi_nodma_device_handle_t handle);



#endif /* MAIN_SPI_MASTER_NODMA_H_ */
