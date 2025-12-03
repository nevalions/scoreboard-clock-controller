#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include <stdbool.h>
#include <stdint.h>

// =====================================================
// LCD Resolution
// =====================================================
#define ST7735_WIDTH 128
#define ST7735_HEIGHT 160

// =====================================================
// Offsets (for ST7735S red-tab / 1.77" common modules)
// =====================================================
#define ST7735_XSTART 0
#define ST7735_YSTART 0

// =====================================================
// Colors (RGB565)
// =====================================================
#define ST7735_BLACK 0x0000
#define ST7735_WHITE 0xFFFF
#define ST7735_RED 0xF800
#define ST7735_GREEN 0x07E0
#define ST7735_BLUE 0x001F
#define ST7735_YELLOW 0xFFE0
#define ST7735_CYAN 0x07FF
#define ST7735_MAGENTA 0xF81F

// =====================================================
// ST7735 Command Set
// =====================================================
#define ST7735_NOP 0x00
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT 0x11
#define ST7735_DISPON 0x29
#define ST7735_CASET 0x2A
#define ST7735_RASET 0x2B
#define ST7735_RAMWR 0x2C
#define ST7735_COLMOD 0x3A
#define ST7735_MADCTL 0x36

// =====================================================
// Struct
// =====================================================
typedef struct {
  spi_device_handle_t spi;
  gpio_num_t cs_pin;
  gpio_num_t dc_pin;
  gpio_num_t rst_pin;
  gpio_num_t mosi_pin;
  gpio_num_t sck_pin;
  uint16_t width;
  uint16_t height;
  bool initialized;
} St7735Lcd;

// =====================================================
// API
// =====================================================
bool st7735_begin(St7735Lcd *lcd, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst,
                  gpio_num_t mosi, gpio_num_t sclk);

void st7735_clear(St7735Lcd *lcd, uint16_t color);
void st7735_set_pixel(St7735Lcd *lcd, uint16_t x, uint16_t y, uint16_t color);
void st7735_draw_rect(St7735Lcd *lcd, int x, int y, int w, int h,
                      uint16_t color);

void st7735_draw_rect_outline(St7735Lcd *lcd, int x, int y, int w, int h,
                              uint16_t color);

void st7735_draw_char(St7735Lcd *lcd, int x, int y, char c, uint16_t fg,
                      uint16_t bg, uint8_t size);

void st7735_print(St7735Lcd *lcd, int x, int y, uint16_t fg, uint16_t bg,
                  uint8_t size, const char *text);

void st7735_printf(St7735Lcd *lcd, int x, int y, uint16_t fg, uint16_t bg,
                   uint8_t size, const char *fmt, ...);

void st7735_test_pattern(St7735Lcd *lcd);

uint16_t st7735_color565(uint8_t r, uint8_t g, uint8_t b);
