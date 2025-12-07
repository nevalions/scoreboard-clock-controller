#include "ui_manager.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_i2c.h"
#include "st7735_lcd.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Include specialized UI modules
#include "ui/ui_helpers.h"
#include "ui/ui_st7735_main.h"
#include "ui/ui_st7735_menus.h"
#include "ui/ui_st7735_variant_bar.h"
#include "ui/ui_lcd_i2c_main.h"
#include "ui/ui_lcd_i2c_menus.h"

static const char *TAG = "UI_MGR";

// -----------------------------------------------------------------------------
// PUBLIC API - Forwarding to specialized modules
// -----------------------------------------------------------------------------
void ui_manager_update_time(UiManager *m, const sport_config_t *sport,
                            uint16_t sec, const SportManager *sm) {
  if (!m || !m->initialized)
    return;

  if (m->type == DISPLAY_TYPE_ST7735)
    ui_st7735_update_time(m, sport, sec, sm);
  else
    ui_draw_i2c_main(m, sport, sec);
}

void ui_manager_update_display(UiManager *m, const sport_config_t *sport,
                               uint16_t sec, const SportManager *sm) {
  if (!m || !m->initialized)
    return;

  if (m->type == DISPLAY_TYPE_ST7735)
    ui_draw_st7735_main(m, sport, sec, sm);
  else
    ui_draw_i2c_main(m, sport, sec);
}

void ui_manager_init_lcd_i2c(UiManager *m, uint8_t addr, gpio_num_t sda,
                             gpio_num_t scl) {
  ESP_LOGI(TAG, "Init I2C LCD 0x%02X", addr);
  m->type = DISPLAY_TYPE_LCD_I2C;

  if (!lcd_i2c_begin(&m->lcd_i2c, addr, sda, scl)) {
    ESP_LOGE(TAG, "LCD init failed");
    m->initialized = false;
    return;
  }

  m->initialized = true;
}

void ui_manager_init_st7735(UiManager *m, gpio_num_t cs, gpio_num_t dc,
                            gpio_num_t rst, gpio_num_t mosi, gpio_num_t sck) {
  ESP_LOGI(TAG, "Init ST7735");
  m->type = DISPLAY_TYPE_ST7735;

  if (!st7735_begin(&m->st7735, cs, dc, rst, mosi, sck)) {
    ESP_LOGE(TAG, "ST7735 init FAILED");
    m->initialized = false;
    return;
  }

  st7735_clear(&m->st7735, ST7735_BLACK);
  ui_draw_st7735_frame(m);

  m->initialized = true;
}

void ui_manager_show_sport_menu(UiManager *m, const sport_group_t *groups,
                                size_t group_count,
                                uint8_t selected_group_idx) {
  if (!m || !m->initialized)
    return;

  if (m->type == DISPLAY_TYPE_ST7735)
    ui_draw_st7735_sport_menu(m, groups, group_count, selected_group_idx);
  else
    ui_draw_i2c_sport_menu(m, groups, group_count, selected_group_idx);
}

void ui_manager_show_variant_menu(UiManager *m, const sport_group_t *group) {
  if (!m || !m->initialized)
    return;

  if (m->type == DISPLAY_TYPE_ST7735)
    ui_draw_st7735_variant_menu(m, group);
  else
    ui_draw_i2c_variant_menu(m, group);
}



void ui_manager_clear(UiManager *m) {
  if (!m || !m->initialized)
    return;

  if (m->type == DISPLAY_TYPE_LCD_I2C)
    lcd_i2c_clear(&m->lcd_i2c);
  else {
    st7735_clear(&m->st7735, ST7735_BLACK);
    ui_draw_st7735_frame(m);
  }
}

void ui_manager_run_lcd_tests(UiManager *m) {
  if (!m || !m->initialized)
    return;

  if (m->type == DISPLAY_TYPE_LCD_I2C) {
    lcd_debug_pin_scan(&m->lcd_i2c);
    vTaskDelay(pdMS_TO_TICKS(1000));
    lcd_debug_all_pins_test(&m->lcd_i2c);
  } else {
    st7735_test_pattern(&m->st7735);
    vTaskDelay(pdMS_TO_TICKS(3000));
    ui_manager_clear(m);
  }
}