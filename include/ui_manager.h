#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sport_manager.h"
#include "sport_selector.h"
#include "st7735_lcd.h"

typedef struct {
  St7735Lcd st7735;
  bool initialized;
} UiManager;

// Initialize ST7735
void ui_manager_init_st7735(UiManager *manager, gpio_num_t cs_pin,
                            gpio_num_t dc_pin, gpio_num_t rst_pin,
                            gpio_num_t mosi_pin, gpio_num_t sck_pin);

// Running-time update: big clock + variant bar
void ui_manager_update_time(UiManager *manager, const sport_config_t *sport,
                            uint16_t seconds,
                            const SportManager *sport_manager);

// Final-5s update: big clock shows tenths ("4.9"), ~10Hz partial redraw
void ui_manager_update_time_tenths(UiManager *manager,
                                   const sport_config_t *sport,
                                   uint16_t deciseconds,
                                   const SportManager *sport_manager);

// Full redraw (state change)
void ui_manager_update_display(UiManager *manager, const sport_config_t *sport,
                               uint16_t seconds,
                               const SportManager *sport_manager);

// Sport selection menu
void ui_manager_show_sport_menu(UiManager *manager, const sport_group_t *groups,
                                size_t group_count, uint8_t selected_group_idx);

void ui_st7735_update_sport_menu_selection(UiManager *manager,
                                           const sport_group_t *groups,
                                           size_t group_count,
                                           uint8_t selected_group_idx);

// Variant selection list
void ui_manager_show_variant_menu(UiManager *manager,
                                  const sport_group_t *group,
                                  uint8_t selected_idx);

// Radio channel menu (occupancy bars + '>' selection + '*' active)
void ui_manager_show_channel_menu(UiManager *manager, const uint8_t *channels,
                                  const uint16_t *scores, uint8_t count,
                                  uint8_t selected_idx, uint8_t active_idx);

// Small RUN/PAUSE + TX-brightness + radio-link status row (running screen
// only); brightness_pct is the profile applied to the transmitted RGB
void ui_manager_draw_status(UiManager *manager, bool running, bool link_good,
                            uint8_t brightness_pct);

void ui_manager_clear(UiManager *manager);
void ui_manager_run_display_tests(UiManager *manager);
