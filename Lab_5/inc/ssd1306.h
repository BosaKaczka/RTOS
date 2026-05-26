#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

// Display geometry
#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  32

// I2C address (0x3C is most common for 128x32)
#define SSD1306_ADDR    0x3C

// SSD1306 commands
#define SSD1306_SET_CONTRAST        0x81
#define SSD1306_DISPLAY_ALL_ON_RESUME 0xA4
#define SSD1306_NORMAL_DISPLAY      0xA6
#define SSD1306_DISPLAY_OFF         0xAE
#define SSD1306_DISPLAY_ON          0xAF
#define SSD1306_SET_DISPLAY_OFFSET  0xD3
#define SSD1306_SET_COM_PINS        0xDA
#define SSD1306_SET_VCOM_DETECT     0xDB
#define SSD1306_SET_DISPLAY_CLOCK   0xD5
#define SSD1306_SET_PRECHARGE       0xD9
#define SSD1306_SET_MULTIPLEX       0xA8
#define SSD1306_SET_LOW_COLUMN      0x00
#define SSD1306_SET_HIGH_COLUMN     0x10
#define SSD1306_SET_START_LINE      0x40
#define SSD1306_MEMORY_MODE         0x20
#define SSD1306_COLUMN_ADDR         0x21
#define SSD1306_PAGE_ADDR           0x22
#define SSD1306_COM_SCAN_DEC        0xC8
#define SSD1306_SEG_REMAP           0xA1
#define SSD1306_CHARGE_PUMP         0x8D

// Frame buffer size: 128 * 32 / 8 bytes
#define SSD1306_BUF_LEN (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

typedef struct {
    uint8_t buf[SSD1306_BUF_LEN];
} ssd1306_t;

void ssd1306_init(ssd1306_t *disp, i2c_inst_t *i2c);
void ssd1306_send_frame(ssd1306_t *disp, i2c_inst_t *i2c);
void ssd1306_clear(ssd1306_t *disp);
void ssd1306_draw_pixel(ssd1306_t *disp, int x, int y, bool on);
void ssd1306_draw_char(ssd1306_t *disp, int x, int y, char c);
void ssd1306_draw_string(ssd1306_t *disp, int x, int y, const char *str);
void ssd1306_draw_line_h(ssd1306_t *disp, int x, int y, int len);

#endif // SSD1306_H
