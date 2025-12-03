#include "ui_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_i2c.h"
#include "st7735_lcd.h"
#include <stdbool.h>
#include <stdint.h>

static const char *TAG = "UI_MGR";

// ============================================================================
// I2C LCD Init
// ============================================================================
void ui_manager_init_lcd_i2c(UiManager *manager, uint8_t i2c_addr,
                             gpio_num_t sda_pin, gpio_num_t scl_pin) {
  if (!manager) {
    ESP_LOGE(TAG, "UI manager pointer is NULL");
    return;
  }

  ESP_LOGI(TAG, "Initializing I2C LCD at address 0x%02X", i2c_addr);
  manager->type = DISPLAY_TYPE_LCD_I2C;

  if (!lcd_i2c_begin(&manager->lcd_i2c, i2c_addr, sda_pin, scl_pin)) {
    ESP_LOGE(TAG, "Failed to initialize I2C LCD");
    manager->initialized = false;
    return;
  }

  manager->initialized = true;
  ESP_LOGI(TAG, "I2C LCD initialized successfully");
}

// ============================================================================
// ST7735 Init
// ============================================================================
void ui_manager_init_st7735(UiManager *manager, gpio_num_t cs_pin,
                            gpio_num_t dc_pin, gpio_num_t rst_pin,
                            gpio_num_t mosi_pin, gpio_num_t sck_pin) {
  if (!manager) {
    ESP_LOGE(TAG, "UI manager pointer is NULL");
    return;
  }

  ESP_LOGI(TAG, "Initializing ST7735 LCD");
  manager->type = DISPLAY_TYPE_ST7735;

  if (!st7735_begin(&manager->st7735, cs_pin, dc_pin, rst_pin, mosi_pin,
                    sck_pin)) {
    ESP_LOGE(TAG, "Failed to initialize ST7735 LCD");
    manager->initialized = false;
    return;
  }

  st7735_clear(&manager->st7735, ST7735_BLACK);

  manager->initialized = true;
  ESP_LOGI(TAG, "ST7735 LCD initialized successfully");
}

// ============================================================================
// Update Display (Main Screen)
// ============================================================================
void ui_manager_update_display(UiManager *manager, const sport_config_t *sport,
                               uint16_t seconds) {
  if (!manager || !manager->initialized) {
    ESP_LOGE(TAG, "UI manager not initialized");
    return;
  }

  if (manager->type == DISPLAY_TYPE_LCD_I2C) {

    lcd_i2c_clear(&manager->lcd_i2c);
    lcd_i2c_set_cursor(&manager->lcd_i2c, 0, 0);
    lcd_i2c_printf(&manager->lcd_i2c, "%s %s", sport->name, sport->variation);

    lcd_i2c_set_cursor(&manager->lcd_i2c, 0, 1);
    lcd_i2c_printf(&manager->lcd_i2c, "Time: %03d", seconds);

  } else if (manager->type == DISPLAY_TYPE_ST7735) {

    st7735_clear(&manager->st7735, ST7735_BLACK);

    // Sport name
    st7735_printf(&manager->st7735, 5, 10, ST7735_WHITE, ST7735_BLACK, 2,
                  "%s %s", sport->name, sport->variation);

    // Timer
    st7735_printf(&manager->st7735, 5, 40, ST7735_WHITE, ST7735_BLACK, 2,
                  "Time: %03d", seconds);

    // Border
    st7735_draw_rect_outline(&manager->st7735, 0, 0, ST7735_WIDTH,
                             ST7735_HEIGHT, ST7735_BLUE);
  }
}

// ============================================================================
// Sport Selection Screen
// ============================================================================
void ui_manager_show_sport_selection(UiManager *manager,
                                     const sport_config_t *selected_sport,
                                     uint16_t current_seconds) {
  if (!manager || !manager->initialized) {
    ESP_LOGE(TAG, "UI manager not initialized");
    return;
  }

  if (manager->type == DISPLAY_TYPE_LCD_I2C) {

    lcd_i2c_clear(&manager->lcd_i2c);
    lcd_i2c_set_cursor(&manager->lcd_i2c, 0, 0);
    lcd_i2c_printf(&manager->lcd_i2c, ">%s %s", selected_sport->name,
                   selected_sport->variation);

    lcd_i2c_set_cursor(&manager->lcd_i2c, 0, 1);
    lcd_i2c_printf(&manager->lcd_i2c, "Time: %03d", current_seconds);

  } else if (manager->type == DISPLAY_TYPE_ST7735) {

    st7735_clear(&manager->st7735, ST7735_BLACK);

    // Highlighted item
    st7735_printf(&manager->st7735, 5, 10, ST7735_GREEN, ST7735_BLACK, 1,
                  ">%s %s", selected_sport->name, selected_sport->variation);

    // Timer below
    st7735_printf(&manager->st7735, 5, 40, ST7735_WHITE, ST7735_BLACK, 2,
                  "Time: %03d", current_seconds);

    // Green selection box
    st7735_draw_rect(&manager->st7735, 2, 8, ST7735_WIDTH - 4, 30,
                     ST7735_GREEN);

    // Outer frame
    st7735_draw_rect(&manager->st7735, 0, 0, ST7735_WIDTH, ST7735_HEIGHT,
                     ST7735_BLUE);
  }
}

// ============================================================================
// Clear Screen
// ============================================================================
void ui_manager_clear(UiManager *manager) {
  if (!manager || !manager->initialized) {
    ESP_LOGE(TAG, "UI manager not initialized");
    return;
  }

  if (manager->type == DISPLAY_TYPE_LCD_I2C)
    lcd_i2c_clear(&manager->lcd_i2c);
  else
    st7735_clear(&manager->st7735, ST7735_BLACK);
}

// ============================================================================
// Tests
// ============================================================================
void ui_manager_run_lcd_tests(UiManager *manager) {
  if (!manager || !manager->initialized) {
    ESP_LOGE(TAG, "UI manager not initialized");
    return;
  }

  if (manager->type == DISPLAY_TYPE_LCD_I2C) {

    ESP_LOGI(TAG, "Running LCD PIN SCAN test!");
    lcd_debug_pin_scan(&manager->lcd_i2c);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Running ALL PINS HIGH/LOW confirmation test...");
    lcd_debug_all_pins_test(&manager->lcd_i2c);

  } else if (manager->type == DISPLAY_TYPE_ST7735) {

    ESP_LOGI(TAG, "Running ST7735 test pattern...");
    st7735_test_pattern(&manager->st7735);
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}
