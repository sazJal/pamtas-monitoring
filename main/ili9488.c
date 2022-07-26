/*
 * ili9488.c
 *
 *  Created on: May 18, 2022
 *      Author: Lenovo
 */

#include "ili9488.h"
//extern uint32_t macgyver[];
extern uint32_t batt[];
extern uint32_t pv[];
extern uint32_t scc[];
extern uint32_t inv[];
extern uint32_t hload[];
extern uint32_t lload[];

display_state_t disp_state;

spi_nodma_device_handle_t spi;
spi_nodma_device_handle_t tsspi;

spi_nodma_bus_config_t buscfg={
    .miso_io_num=PIN_NUM_MISO,
    .mosi_io_num=PIN_NUM_MOSI,
    .sclk_io_num=PIN_NUM_CLK,
    .quadwp_io_num=-1,
    .quadhd_io_num=-1
};

spi_nodma_device_interface_config_t devcfg={
    .clock_speed_hz=20000000,                //Initial clock out at 8 MHz
    .mode=0,                                //SPI mode 0
    .spics_io_num=-1,                       //we will use external CS pin
	.spics_ext_io_num=PIN_NUM_CS,           //external CS pin
	.flags=SPI_DEVICE_HALFDUPLEX,           //Set half duplex mode (Full duplex mode can also be set by commenting this line
											// but we don't need full duplex in  this example
											// also, SOME OF TFT FUNCTIONS ONLY WORKS IN HALF DUPLEX MODE
	.queue_size = 2,						// in some tft functions we are using DMA mode, so we need queues!
	// We can use pre_cb callback to activate DC, but we are handling DC in low-level functions, so it is not necessary
    //.pre_cb=ili_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
};

const uint8_t ILI9488_init[]= {
  18,                   					        // 18 commands in list
  ILI9341_SWRESET, DELAY,   						//  1: Software reset, no args, w/delay
  200,												//     200 ms delay

  0xE0, 15, 0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78, 0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F,
  0xE1, 15,	0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45, 0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F,
  0xC0, 2,   //Power Control 1
	0x17,    //Vreg1out
	0x15,    //Verg2out

  0xC1, 1,   //Power Control 2
	0x41,    //VGH,VGL

  0xC5, 3,   //Power Control 3
	0x00,
	0x12,    //Vcom
	0x80,

  TFT_MADCTL, 1,									// Memory Access Control (orientation)
    (MADCTL_MV | MADCTL_BGR),
//  0x48,
//    0x08,

  // *** INTERFACE PIXEL FORMAT: 0x66 -> 18 bit; 0x55 -> 16 bit
  ILI9341_PIXFMT, 1, 0x66,

  0xB0, 1,   // Interface Mode Control
	0x00,    // 0x80: SDO NOT USE; 0x00 USE SDO

  0xB1, 1,   //Frame rate
	0xA0,    //60Hz

  0xB4, 1,   //Display Inversion Control
	0x02,    //2-dot

  0xB6, 2,   //Display Function Control  RGB/MCU Interface Control
	0x02,    //MCU
	0x02,    //Source,Gate scan direction
//  0xB6, 3,   //Display Function Control  RGB/MCU Interface Control
//	0x02,    //MCU
//	0x02,    //Source,Gate scan direction
//	0x3B,

  0xE9, 1,   // Set Image Function
	0x00,    // Disable 24 bit data

  0x53, 1,   // Write CTRL Display Value
	0x28,    // BCTRL && DD on

  0x51, 1,   // Write Display Brightness Value
	0x05,    //

  0xF7, 4,   // Adjust Control
	0xA9,
	0x51,
	0x2C,
	0x02,    // D7 stream, loose
//	0x82,

  0x11, DELAY,  //Exit Sleep
    120,
  0x29, 0,      //Display on

};

void commandList(spi_nodma_device_handle_t spi, const uint8_t *addr) {
  uint8_t  numCommands, numArgs, cmd;
  uint16_t ms;

  numCommands = *addr++;         // Number of commands to follow
  while(numCommands--) {         // For each command...
    cmd = *addr++;               // save command
    numArgs  = *addr++;          //   Number of args to follow
    ms       = numArgs & DELAY;  //   If high bit set, delay follows args
    numArgs &= ~DELAY;           //   Mask out delay bit

	disp_spi_transfer_cmd_data(cmd, (uint8_t *)addr, numArgs);

	addr += numArgs;

    if(ms) {
      ms = *addr++;              // Read post-command delay time (ms)
      if(ms == 255) ms = 500;    // If 255, delay for 500 ms
	  vTaskDelay(ms / portTICK_RATE_MS);
    }
  }
}

void ili_init(spi_nodma_device_handle_t spi)
{
    esp_err_t ret;
    uint8_t tft_pix_fmt = DISP_COLOR_BITS_24;
    uint8_t *line_buf;
    line_buf = malloc(_width*sizeof(color_t)+1);

	tft_line = malloc((TFT_LINEBUF_MAX_SIZE*3) + 1);
	assert(tft_line);

    //Initialize non-SPI GPIOs
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);

	gpio_set_direction(PIN_NUM_MISO, GPIO_MODE_INPUT);
	gpio_set_pull_mode(PIN_NUM_MISO, GPIO_PULLUP_ONLY);
	gpio_pad_select_gpio(GPIO_NUM_2);
	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
	gpio_set_level(GPIO_NUM_2, 1);

	vTaskDelay(500 / portTICK_RATE_MS);

    ret=spi_nodma_bus_add_device(SPI_BUS, &buscfg, &devcfg, &spi);
    assert(ret==ESP_OK);
	printf("SPI: display device added to spi bus\r\n");
	disp_spi = spi;

	// Test select/deselect
	ret = spi_nodma_device_select(spi, 1);
    assert(ret==ESP_OK);
	ret = spi_nodma_device_deselect(spi);
    assert(ret==ESP_OK);

#if PIN_NUM_BCKL
    gpio_set_direction(PIN_NUM_BCKL, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_BCKL, PIN_BCKL_OFF);
#endif

#if PIN_NUM_RST
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    //Reset the display
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(100 / portTICK_RATE_MS);
#endif

    //Send all the initialization commands
	if (tft_disp_type == DISP_TYPE_ILI9341) {
		_width = ILI9341_HEIGHT;
		_height =ILI9341_WIDTH;
		ret = disp_select();
		assert(ret==ESP_OK);
//		commandList(spi, ILI9341_init);
	}
	else if (tft_disp_type == DISP_TYPE_ILI9488) {
		COLOR_BITS = 24;  // only 18-bit color format supported
		_width = ILI9488_HEIGHT;
		_height = ILI9488_WIDTH;
		ret = disp_select();
		assert(ret==ESP_OK);
		commandList(spi, ILI9488_init);
	}
	else assert(0);


    if ((COLOR_BITS == 16) && (tft_disp_type == DISP_TYPE_ILI9341)) tft_pix_fmt = DISP_COLOR_BITS_16;
    disp_spi_transfer_cmd_data(ILI9341_PIXFMT, &tft_pix_fmt, 1);

    ret = disp_deselect();
	assert(ret==ESP_OK);

	///Enable backlight
#if PIN_NUM_BCKL
    gpio_set_level(PIN_NUM_BCKL, PIN_BCKL_ON);
#endif
}

void draw_first_page(void)
{
	/* Draw Boxes */
	TFT_drawRect(9,   35,  150,  283, TFT_WHITE); // source Box
	TFT_drawRect(158, 35,  313,  120, TFT_WHITE); // ac output Box
	TFT_drawRect(158, 154, 313,  60, TFT_WHITE); // beban radio Box
	TFT_drawRect(158, 213, 313,  105, TFT_WHITE); // beban fasilitas Box

	_fg = TFT_WHITE;
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("PT. Len Bandung", 150, 4);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("2022/07/20 - 15:18", 320, 5);

	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("SUMBER", 50, 45);
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("AC OUTPUT", 275, 45);

	/* SUMBER */
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Tegangan PV :", 20, 70);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Arus PV :", 20, 110);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Daya PV :", 20, 150);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Teg. Baterai :", 20, 190);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Arus Baterai :", 20, 230);

	/* SUMBER VALUE */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 44, 90);
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 44, 130);
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 44, 170);
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 44, 210);
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 44, 250);

	/* SUMBER UNIT */
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("V", 130, 90);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("A", 130, 130);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("W", 130, 170);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("V", 130, 210);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("A", 130, 250);


	/* AC OUTPUT */
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Tegangan :", 175, 70);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Arus :", 175, 110);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Frekuensi :", 330, 70);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Daya :", 330, 110);

	/* AC OUTPUT VALUE */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 199, 90);
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 199, 130);
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 354, 90);
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 354, 130);

	/* AC OUTPUT UNIT */
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("V", 285, 90);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("A", 285, 130);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Hz", 440, 90);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("H", 440, 130);

	/* BEBAN RADIO */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("Beban Radio", 175, 165);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Energi Terpakai", 175, 190);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print(":", 318, 190);

	/* BEBAN RADIO VALUE */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 327, 190);

	/* BEBAN RADIO UNIT */
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Wh", 440, 190);

	/* BEBAN UTILITAS */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("Beban Fasilitas", 175, 225);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Energi Terpakai", 175, 250);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Sisa Energi", 175, 270);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Kuota Energi", 175, 290);
	TFT_print(":", 318, 250);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print(":", 318, 270);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print(":", 318, 290);

	/* BEBAN UTILITAS VALUE */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 327, 250);
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 327, 270);
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 327, 290);

	/* BEBAN UTILITAS UNIT */
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Wh", 440, 250);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Wh", 440, 270);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Wh", 440, 290);

	draw_battery_cover();
	draw_battery_point(2);
}

void show_first_page(pzem_t data, float limit, float sisa, source_param_t source_param)
{
	char buffData[10];
	clear_values_first();
	TFT_setFont(DEJAVU18_FONT, NULL);
	// Source
	_fg = TFT_GAME_A;
	sprintf(buffData,"%.2f", source_param.pv_voltage);
	TFT_print(buffData, 44, 90);
	sprintf(buffData,"%.2f", source_param.pv_current);
	TFT_print(buffData, 44, 130);
	sprintf(buffData,"%.2f", source_param.pv_power);
	TFT_print(buffData, 44, 170);
	sprintf(buffData,"%.2f", source_param.batt_voltage);
	TFT_print(buffData, 44, 210);
	sprintf(buffData,"%.2f", source_param.batt_current);
	TFT_print(buffData, 44, 250);

	/* AC Output */
	_fg = TFT_GAME_A;
	sprintf(buffData,"%.2f", data.data[0].voltage);
	TFT_print(buffData, 199, 90);

	sprintf(buffData,"%.2f", data.data[0].current);
	TFT_print(buffData, 199, 130);

	sprintf(buffData,"%.2f", data.data[0].frequency);
	TFT_print(buffData, 354, 90);

	sprintf(buffData,"%.2f", data.data[0].power);
	TFT_print(buffData, 354, 130);

	/* Beban Radio Energy */
	sprintf(buffData,"%.2f", data.data[0].energy);
	TFT_print(buffData, 327, 190);

	/* Beban Utility Energy */
	sprintf(buffData,"%.2f", data.data[1].energy);
	TFT_print(buffData, 327, 250);

	sprintf(buffData,"%.2f", sisa);
	TFT_print(buffData, 327, 270);

	sprintf(buffData,"%.2f", limit);
	TFT_print(buffData, 327, 290);

}

void clean_first_page(void)
{
	/* Clean any value */
	clear_values_first();

	/* Clean Boxes */
	TFT_drawRect(9,   35,  150,  283, TFT_BLACK); // source Box
	TFT_drawRect(158, 35,  313,  120, TFT_BLACK); // ac output Box
	TFT_drawRect(158, 154, 313,  60, TFT_BLACK); // beban radio Box
	TFT_drawRect(158, 213, 313,  105, TFT_BLACK); // beban fasilitas Box

	_fg = TFT_BLACK;

	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("SUMBER", 50, 45);
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("AC OUTPUT", 275, 45);

	/* SUMBER */
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Tegangan PV :", 20, 70);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Arus PV :", 20, 110);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Daya PV :", 20, 150);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Teg. Baterai :", 20, 190);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Arus Baterai :", 20, 230);

	/* SUMBER UNIT */
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("V", 130, 90);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("A", 130, 130);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("W", 130, 170);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("V", 130, 210);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("A", 130, 250);

	/* AC OUTPUT */
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Tegangan :", 175, 70);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Arus :", 175, 110);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Frekuensi :", 330, 70);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Daya :", 330, 110);

	/* AC OUTPUT UNIT */
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("V", 285, 90);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("A", 285, 130);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Hz", 440, 90);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("H", 440, 130);

	/* BEBAN RADIO */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("Beban Radio", 175, 165);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Energi Terpakai", 175, 190);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print(":", 318, 190);

	/* BEBAN RADIO UNIT */
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Wh", 440, 190);

	/* BEBAN UTILITAS */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("Beban Fasilitas", 175, 225);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Energi Terpakai", 175, 250);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Sisa Energi", 175, 270);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Kuota Energi", 175, 290);
	TFT_print(":", 318, 250);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print(":", 318, 270);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print(":", 318, 290);

	/* BEBAN UTILITAS UNIT */
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Wh", 440, 250);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Wh", 440, 270);
	TFT_setFont(UBUNTU16_FONT, NULL);
	TFT_print("Wh", 440, 290);
}

void draw_second_page(void)
{
	_fg = TFT_WHITE;

	/* image creation */
	TFT_pushColorImageRep(30,   45,  107,  70,    pv, (78*26));
	TFT_pushColorImageRep(50,  160,   85, 195,   scc, (36*36));
	TFT_pushColorImageRep(250, 160,  285, 195,   inv, (36*36));
	TFT_pushColorImageRep(360,  80,  395, 115, hload, (36*36));
	TFT_pushColorImageRep(360, 240,  395, 275, lload, (36*36));
	TFT_pushColorImageRep(150, 240,  177, 291,  batt, (28*52));

	/* line creation */
	TFT_drawFastVLine(63,   71,  90, TFT_WHITE); // PV-SCC
	TFT_drawFastHLine(86,  178, 165, TFT_WHITE); // SCC-INV
	TFT_drawFastVLine(164, 178,  62, TFT_WHITE); // SCC-BATT
	TFT_drawFastHLine(286, 178,  34, TFT_WHITE); // INV-LOAD
	TFT_drawFastVLine(320,  98, 160, TFT_WHITE); // INV-LOAD
	TFT_drawFastHLine(321,  98,  40, TFT_WHITE); // INV-HLOAD
	TFT_drawFastHLine(321, 258,  40, TFT_WHITE); // INV-HLOAD

	_fg = TFT_WHITE;

	/* P PV */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 80, 85);
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("W", 165, 85);
	/* P SCC */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 95, 150);
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("W", 180, 150);
	/* P BAT */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 175, 215);
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("W", 260, 215);
	/* P HLOAD */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 330, 125);
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("W", 415, 125);
	/* P LLOAD */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("0.0", 330, 215);
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("W", 415, 215);

	draw_battery_cover();
	draw_battery_point(4);
}

void show_second_page(pzem_t data, source_param_t source_param)
{
	char buffData[10];
	clear_values_second();
	TFT_setFont(DEJAVU18_FONT, NULL);
	// Source
	_fg = TFT_WHITE;

	/* P PV */
	sprintf(buffData,"%.2f", source_param.pv_power);
	TFT_print(buffData, 80, 85);

	/* P SCC */
	sprintf(buffData,"%.2f", source_param.batt_power);
	TFT_print(buffData, 95, 150);

	/* P BAT */
	sprintf(buffData,"%.2f", source_param.pv_current);
	TFT_print(buffData, 175, 215);

	/* P HLOAD */
	sprintf(buffData,"%.2f", data.data[0].power);
	TFT_print(buffData, 330, 125);

	/* P LLOAD */
	sprintf(buffData,"%.2f", data.data[1].power);
	TFT_print(buffData, 330, 215);
}

void clean_second_page(void)
{
	/* Clean any value */
	clear_values_second();

	/* image creation */
	TFT_fillRect(30, 45, 107, 70, TFT_BLACK);
	TFT_fillRect(50, 160, 85, 195, TFT_BLACK);
	TFT_fillRect(250, 160, 285, 195, TFT_BLACK);
	TFT_fillRect(360, 80, 395, 115, TFT_BLACK);
	TFT_fillRect(360, 240, 395, 275, TFT_BLACK);
	TFT_fillRect(150, 240, 177, 291, TFT_BLACK);

	/* line creation */
	TFT_drawFastVLine(63, 71, 90, TFT_BLACK); // PV-SCC
	TFT_drawFastHLine(86, 178, 165, TFT_BLACK); // SCC-INV
	TFT_drawFastVLine(164, 178, 62, TFT_BLACK); // SCC-BATT
	TFT_drawFastHLine(286, 178, 34, TFT_BLACK); // INV-LOAD
	TFT_drawFastVLine(320, 98, 160, TFT_BLACK); // INV-LOAD
	TFT_drawFastHLine(321, 98, 40, TFT_BLACK); // INV-HLOAD
	TFT_drawFastHLine(321, 258, 40, TFT_BLACK); // INV-HLOAD

	_fg = TFT_BLACK;
	/* P PV */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("W", 165, 85);
	/* P SCC */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("W", 180, 150);
	/* P BAT */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("W", 260, 215);
	/* P HLOAD */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("W", 415, 125);
	/* P LLOAD */
	TFT_setFont(DEJAVU18_FONT, NULL);
	TFT_print("W", 415, 215);
}

void clear_values_first(void)
{
	static int x, y;
	TFT_setFont(DEJAVU18_FONT, NULL);
	tft_getfontsize(&x, &y);
	_fg = TFT_GAME_B;
	/* Clear Sumber value */
	TFT_fillRect(44, 	90, 85, y, TFT_BLACK); // x= 17*6
	TFT_fillRect(44,   130, 85, y, TFT_BLACK);
	TFT_fillRect(44,   170, 85, y, TFT_BLACK);
	TFT_fillRect(44,   210, 85, y, TFT_BLACK);
	TFT_fillRect(44,   250, 85, y, TFT_BLACK);

	/* Clear AC Output value */
	TFT_fillRect(199,   90, 85, y, TFT_BLACK);
	TFT_fillRect(199,  130, 85, y, TFT_BLACK);
	TFT_fillRect(354,   90, 85, y, TFT_BLACK);
	TFT_fillRect(354,  130, 85, y, TFT_BLACK);

	/* Clear Beban Radio value */
	TFT_fillRect(327,   190, 102, y, TFT_BLACK);

	/* Clear Beban Fasilitas value */
	TFT_fillRect(327,  250, 102, y, TFT_BLACK);
	TFT_fillRect(327,  270, 102, y, TFT_BLACK);
	TFT_fillRect(327,  290, 102, y, TFT_BLACK);
}

void clear_values_second(void)
{
	static int x, y;
	TFT_setFont(DEJAVU18_FONT, NULL);
	tft_getfontsize(&x, &y);
	_fg = TFT_GAME_B;

	/* Clear P PV Value */
	TFT_fillRect(80, 	85, 85, y, TFT_BLACK); // x= 17*5

	/* Clear P SCC Value */
	TFT_fillRect(95,   150, 85, y, TFT_BLACK); // x= 17*5

	/* Clear P Bat. Value */
	TFT_fillRect(175,  215, 85, y, TFT_BLACK); // x= 17*5

	/* Clear P Radio Load Value */
	TFT_fillRect(330,  125, 85, y, TFT_BLACK); // x= 17*5

	/* Clear P Utility Load */
	TFT_fillRect(330,  215, 85, y, TFT_BLACK); // x= 17*5
}

void clear_first_page_variables(void)
{
	static int x, y;
	_fg = TFT_GAME_B;
	tft_getfontsize(&x, &y);

	TFT_fillRect(27, 85, x<<3, y, TFT_GAME_B);
	TFT_fillRect(27, 125, x<<3, y, TFT_GAME_B);
	TFT_fillRect(27, 165, x<<3, y, TFT_GAME_B);
	TFT_fillRect(27, 205, x<<3, y, TFT_GAME_B);
	TFT_fillRect(27, 245, x<<3, y, TFT_GAME_B);
	TFT_fillRect(27, 285, x<<3, y, TFT_GAME_B);

	TFT_fillRect(182, 85, x<<3, y, TFT_GAME_B);
	TFT_fillRect(182, 125, x<<3, y, TFT_GAME_B);
	TFT_fillRect(182, 165, x<<3, y, TFT_GAME_B);
	TFT_fillRect(182, 205, x<<3, y, TFT_GAME_B);

	TFT_fillRect(337, 85, x<<3, y, TFT_GAME_B);
	TFT_fillRect(337, 125, x<<3, y, TFT_GAME_B);
	TFT_fillRect(337, 165, x<<3, y, TFT_GAME_B);
	TFT_fillRect(337, 205, x<<3, y, TFT_GAME_B);

	TFT_fillRect(182, 285, x<<3, y, TFT_GAME_B);
	TFT_fillRect(337, 285, x<<3, y, TFT_GAME_B);
}

void draw_battery_cover(void)
{
	color_t color = TFT_WHITE;
	TFT_drawRect(9, 5, 58, 20, color);
	TFT_fillRect(67, 9, 4, 12, color);
}

void draw_battery_point(uint8_t level)
{
	color_t color = TFT_GAME_A;

	switch(level)
	{
	case 0	: 	color = TFT_BLACK;
				TFT_fillRect(12, 9, 8, 12, color);
				TFT_fillRect(23, 9, 8, 12, color);
				TFT_fillRect(34, 9, 8, 12, color);
				TFT_fillRect(45, 9, 8, 12, color);
				TFT_fillRect(56, 9, 8, 12, color);
				break;
	case 1	: 	color = TFT_GREEN;
				TFT_fillRect(12, 9, 8, 12, color);
				color = TFT_BLACK;
				TFT_fillRect(23, 9, 8, 12, color);
				TFT_fillRect(34, 9, 8, 12, color);
				TFT_fillRect(45, 9, 8, 12, color);
				TFT_fillRect(56, 9, 8, 12, color);
				break;
	case 2	: 	color = TFT_GREEN;
				TFT_fillRect(12, 9, 8, 12, color);
				TFT_fillRect(23, 9, 8, 12, color);
				color = TFT_BLACK;
				TFT_fillRect(34, 9, 8, 12, color);
				TFT_fillRect(45, 9, 8, 12, color);
				TFT_fillRect(56, 9, 8, 12, color);
				break;
	case 3	: 	color = TFT_GREEN;
				TFT_fillRect(12, 9, 8, 12, color);
				TFT_fillRect(23, 9, 8, 12, color);
				TFT_fillRect(34, 9, 8, 12, color);
				color = TFT_BLACK;
				TFT_fillRect(45, 9, 8, 12, color);
				TFT_fillRect(56, 9, 8, 12, color);
				break;
	case 4	: 	color = TFT_GREEN;
				TFT_fillRect(12, 9, 8, 12, color);
				TFT_fillRect(23, 9, 8, 12, color);
				TFT_fillRect(34, 9, 8, 12, color);
				TFT_fillRect(45, 9, 8, 12, color);
				color = TFT_BLACK;
				TFT_fillRect(56, 9, 8, 12, color);
				break;
	case 5	: 	color = TFT_GREEN;
				TFT_fillRect(12, 9, 8, 12, color);
				TFT_fillRect(23, 9, 8, 12, color);
				TFT_fillRect(34, 9, 8, 12, color);
				TFT_fillRect(45, 9, 8, 12, color);
				TFT_fillRect(56, 9, 8, 12, color);
				break;
	default : 	color = TFT_BLACK;
				TFT_fillRect(12, 9, 8, 12, color);
				TFT_fillRect(23, 9, 8, 12, color);
				TFT_fillRect(34, 9, 8, 12, color);
				TFT_fillRect(45, 9, 8, 12, color);
				TFT_fillRect(56, 9, 8, 12, color);
				break;
	}

}

void draw_up_triangle(void)
{
	TFT_fillTriangle(283, 303, 288, 295, 293, 303, TFT_YELLOW);
}

void draw_down_triangle(void)
{
	TFT_fillTriangle(292, 295, 297, 303, 302, 295, TFT_YELLOW);
}

void erase_up_triangle(void)
{
	TFT_fillTriangle(283, 303, 288, 295, 293, 303, TFT_BLACK);
}

void erase_down_triangle(void)
{
	TFT_fillTriangle(292, 295, 297, 303, 302, 295, TFT_BLACK);
}

void draw_edit_box(void)
{
	TFT_drawRect(170, 285, 266, 28, TFT_RED);
	TFT_drawRect(171, 286, 264, 26, TFT_RED);
}

void erase_edit_box(void)
{
	TFT_drawRect(170, 285, 266, 28, TFT_BLACK);
	TFT_drawRect(171, 286, 264, 26, TFT_BLACK);
}
