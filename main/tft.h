/*
 * tft.h
 *
 *  Created on: May 23, 2022
 *      Author: Lenovo
 */

#ifndef MAIN_TFT_H_
#define MAIN_TFT_H_

#include "tftfunc.h"

// Constants for ellipse function
#define TFT_ELLIPSE_UPPER_RIGHT 0x01
#define TFT_ELLIPSE_UPPER_LEFT  0x02
#define TFT_ELLIPSE_LOWER_LEFT  0x04
#define TFT_ELLIPSE_LOWER_RIGHT 0x08

// Constants for Arc function
// number representing the maximum angle (e.g. if 100, then if you pass in start=0 and end=50, you get a half circle)
// this can be changed with setArcParams function at runtime
#define DEFAULT_ARC_ANGLE_MAX 360
// rotational offset in degrees defining position of value 0 (-90 will put it at the top of circle)
// this can be changed with setAngleOffset function at runtime
#define DEFAULT_ANGLE_OFFSET -90

// Color definitions constants
const color_t TFT_BLACK;
const color_t TFT_NAVY;
const color_t TFT_DARKGREEN;
const color_t TFT_DARKCYAN;
const color_t TFT_MAROON;
const color_t TFT_PURPLE;
const color_t TFT_OLIVE;
const color_t TFT_LIGHTGREY;
const color_t TFT_DARKGREY;
const color_t TFT_BLUE;
const color_t TFT_GREEN;
const color_t TFT_CYAN;
const color_t TFT_RED;
const color_t TFT_MAGENTA;
const color_t TFT_YELLOW;
const color_t TFT_WHITE;
const color_t TFT_ORANGE;
const color_t TFT_GREENYELLOW;
const color_t TFT_PINK;

const color_t TFT_GAME_A;
const color_t TFT_GAME_B;

#define INVERT_ON		1
#define INVERT_OFF		0

// Screen orientation constants
#define PORTRAIT	0
#define LANDSCAPE	1
#define PORTRAIT_FLIP	2
#define LANDSCAPE_FLIP	3

// Special coordinates constants
#define LASTX	-1
#define LASTY	-2
#define CENTER	-3
#define RIGHT	-4
#define BOTTOM	-4

// Embeded fonts constants
#define DEFAULT_FONT	0
#define DEJAVU18_FONT	1
#define DEJAVU24_FONT	2
#define UBUNTU16_FONT	3
#define COMIC24_FONT	4
#define MINYA24_FONT	5
#define TOONEY32_FONT	6
#define FONT_7SEG		7
#define USER_FONT		8  // font will be read from file


uint8_t     orientation;    // current screen orientation
uint16_t    rotation;       // current font rotation angle (0~395)
uint8_t     _transparent;   // if not 0 draw fonts transparent
color_t     _fg;            // current foreground color for fonts
color_t     _bg;            // current background for non transparent fonts
uint8_t     _wrap;          // if not 0 wrap long text to the new line, else clip
uint8_t     _forceFixed;    // if not zero force drawing proportional fonts with fixed width

void TFT_drawPixel(int16_t x, int16_t y, color_t color, uint8_t sel);
color_t TFT_readPixel(int16_t x, int16_t y);
void TFT_drawFastVLine(int16_t x, int16_t y, int16_t h, color_t color);
void TFT_drawFastHLine(int16_t x, int16_t y, int16_t w, color_t color);
void TFT_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color);
void TFT_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, color_t color);
void TFT_drawRect(uint16_t x1,uint16_t y1,uint16_t w,uint16_t h, color_t color);
void TFT_drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r, color_t color);
void TFT_fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r, color_t color);
void TFT_fillScreen(color_t color);
void TFT_drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, color_t color);
void TFT_fillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, color_t color);
void TFT_drawCircle(int16_t x, int16_t y, int radius, color_t color);
void TFT_fillCircle(int16_t x, int16_t y, int radius, color_t color);
void TFT_draw_ellipse(uint16_t x0, uint16_t y0, uint16_t rx, uint16_t ry, color_t color, uint8_t option);
void TFT_draw_filled_ellipse(uint16_t x0, uint16_t y0, uint16_t rx, uint16_t ry, color_t color, uint8_t option);
int compare_colors(color_t c1, color_t c2);

void drawPolygon(int cx, int cy, int sides, int diameter, color_t color, uint8_t fill, int deg);
void drawStar(int cx, int cy, int diameter, color_t color, bool fill, float factor);

void TFT_setFont(uint8_t font, const char *font_file);

void TFT_print(char *st, int x, int y);

int tft_getfontsize(int *width, int* height);
int tft_getfontheight();

void tft_setclipwin(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

void tft_resetclipwin();

void set_font_atrib(uint8_t l, uint8_t w, int offset, color_t color);

void TFT_setRotation(uint8_t m);
void TFT_invertDisplay(const uint8_t mode);

int getStringWidth(char* str);
color_t HSBtoRGB(float _hue, float _sat, float _brightness);
void tft_jpg_image(int x, int y, int ascale, char *fname, uint8_t *buf, int size);
int tft_bmp_image(int x, int y, char *fname, uint8_t *imgbuf, int size);
int tft_read_touch(int *x, int* y, uint8_t raw);



#endif /* MAIN_TFT_H_ */
