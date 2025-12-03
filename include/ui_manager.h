#pragma once

#include "../../sport_selector/include/sport_selector.h"
#include "lcd_i2c.h"
#include "st7735_lcd.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum { DISPLAY_TYPE_LCD_I2C, DISPLAY_TYPE_ST7735 } display_type_t;

typedef struct {
  display_type_t type;
  union {
    LcdI2C lcd_i2c;
    St7735Lcd st7735;
  };
  bool initialized;
} UiManager;

void ui_manager_init_lcd_i2c(UiManager *manager, uint8_t i2c_addr,
                             gpio_num_t sda_pin, gpio_num_t scl_pin);

void ui_manager_init_st7735(UiManager *manager, gpio_num_t cs_pin,
                            gpio_num_t dc_pin, gpio_num_t rst_pin,
                            gpio_num_t mosi_pin, gpio_num_t sck_pin);

void ui_manager_update_time(UiManager *manager, const sport_config_t *sport,
                            uint16_t seconds);

void ui_manager_update_display(UiManager *manager, const sport_config_t *sport,
                               uint16_t seconds);

void ui_manager_show_sport_selection(UiManager *manager,
                                     const sport_config_t *selected_sport,
                                     uint16_t current_seconds);

void ui_manager_clear(UiManager *manager);
void ui_manager_run_lcd_tests(UiManager *manager);
