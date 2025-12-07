#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "st7735_lcd.h"
#include "ui_manager.h"

// Layout constants
#define UI_I2C_LINE_TIME_COL 0
#define UI_ST7735_MARGIN 2
#define UI_ST7735_HEADER_Y 10
#define UI_ST7735_VARIANT_BAR_Y 30
#define UI_ST7735_VARIANT_BAR_SPACING 30
#define UI_ST7735_VARIANT_HIGHLIGHT_COLOR ST7735_YELLOW
#define UI_ST7735_VARIANT_NORMAL_COLOR ST7735_WHITE
#define UI_ST7735_VARIANT_LIST_Y 70
#define UI_ST7735_VARIANT_SPACING 22
#define UI_ST7735_MENU_LIST_Y 60
#define UI_ST7735_LINE_SPACING 10

// Helper functions
void ui_format_seconds(char *out, size_t size, uint16_t sec);
void ui_st7735_print_center(St7735Lcd *lcd, int y, uint16_t fg, uint16_t bg,
                            uint8_t size, const char *text);
void ui_draw_st7735_header_underline(St7735Lcd *lcd);
void ui_draw_st7735_frame(UiManager *m);