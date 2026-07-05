#include "ui_manager.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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

static const char *TAG = "UI_MGR";

// -----------------------------------------------------------------------------
// PUBLIC API - Forwarding to specialized modules
// -----------------------------------------------------------------------------
void ui_manager_update_time(UiManager *m, const sport_config_t *sport,
                            uint16_t sec, const SportManager *sm) {
  if (!m || !m->initialized)
    return;

  ui_st7735_update_time(m, sport, sec, sm);
}

void ui_manager_update_display(UiManager *m, const sport_config_t *sport,
                               uint16_t sec, const SportManager *sm) {
  if (!m || !m->initialized)
    return;

  ui_draw_st7735_main(m, sport, sec, sm);
}

void ui_manager_init_st7735(UiManager *m, gpio_num_t cs, gpio_num_t dc,
                            gpio_num_t rst, gpio_num_t mosi, gpio_num_t sck) {
  ESP_LOGI(TAG, "Init ST7735");

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

  ui_draw_st7735_sport_menu(m, groups, group_count, selected_group_idx);
}

void ui_manager_show_variant_menu(UiManager *m, const sport_group_t *group,
                                  uint8_t selected_idx) {
  if (!m || !m->initialized)
    return;

  ui_draw_st7735_variant_menu(m, group, selected_idx);
}

void ui_manager_update_time_tenths(UiManager *m, const sport_config_t *sport,
                                   uint16_t deciseconds,
                                   const SportManager *sport_manager) {
  if (!m || !m->initialized)
    return;

  ui_st7735_update_time_tenths(m, sport, deciseconds, sport_manager);
}

void ui_manager_draw_status(UiManager *m, bool running, bool link_good,
                            uint8_t brightness_pct) {
  if (!m || !m->initialized)
    return;

  ui_st7735_draw_status(m, running, link_good, brightness_pct);
}

void ui_manager_clear(UiManager *m) {
  if (!m || !m->initialized)
    return;

  st7735_clear(&m->st7735, ST7735_BLACK);
  ui_draw_st7735_frame(m);
}

void ui_manager_run_display_tests(UiManager *m) {
  if (!m || !m->initialized)
    return;

  st7735_test_pattern(&m->st7735);
  vTaskDelay(pdMS_TO_TICKS(3000));
  ui_manager_clear(m);
}
