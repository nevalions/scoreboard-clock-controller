#pragma once

#include "../include/lcd_i2c.h"
#include "../../sport_selector/include/sport_selector.h"

typedef struct {
    LcdI2C lcd;
} UiManager;

void ui_manager_init(UiManager *manager, uint8_t i2c_addr, gpio_num_t sda_pin, gpio_num_t scl_pin);
void ui_manager_update_display(UiManager *manager, const sport_config_t *sport, uint16_t seconds);
void ui_manager_show_sport_selection(UiManager *manager, const sport_config_t *selected_sport, uint16_t current_seconds);
void ui_manager_clear(UiManager *manager);
void ui_manager_run_lcd_tests(UiManager *manager);