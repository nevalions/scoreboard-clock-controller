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

static const char *TAG = "UI_MGR";

// -----------------------------------------------------------------------------
// Layout / style constants
// -----------------------------------------------------------------------------
#define UI_I2C_LINE_TIME_COL 0

#define UI_ST7735_MARGIN 2
#define UI_ST7735_HEADER_Y 8
#define UI_ST7735_TIMER_Y 40
#define UI_ST7735_SELECTION_BOX_Y 22
#define UI_ST7735_SELECTION_BOX_H 28

// -----------------------------------------------------------------------------
// Helpers - ST7735 text centering
// -----------------------------------------------------------------------------
static void st7735_print_center(St7735Lcd *lcd, int y, uint16_t fg, uint16_t bg,
                                uint8_t size, const char *text) {
  if (!lcd || !lcd->initialized || !text) {
    return;
  }

  int len = strlen(text);
  if (len <= 0) {
    return;
  }

  int text_width = len * 8 * size; // 8px per glyph
  int x = (lcd->width - text_width) / 2;
  if (x < 0) {
    x = 0; // fallback if text is wider than screen
  }

  st7735_print(lcd, x, y, fg, bg, size, text);
}

// -----------------------------------------------------------------------------
// Helpers - I2C LCD drawing
// -----------------------------------------------------------------------------
static void ui_draw_i2c_main(UiManager *manager, const sport_config_t *sport,
                             uint16_t seconds) {
  lcd_i2c_clear(&manager->lcd_i2c);
  lcd_i2c_set_cursor(&manager->lcd_i2c, 0, 0);
  lcd_i2c_printf(&manager->lcd_i2c, "%s %s", sport->name, sport->variation);

  lcd_i2c_set_cursor(&manager->lcd_i2c, UI_I2C_LINE_TIME_COL, 1);
  lcd_i2c_printf(&manager->lcd_i2c, "Time: %03d", seconds);
}

static void ui_draw_i2c_sport_selection(UiManager *manager,
                                        const sport_config_t *selected_sport,
                                        uint16_t current_seconds) {
  lcd_i2c_clear(&manager->lcd_i2c);
  lcd_i2c_set_cursor(&manager->lcd_i2c, 0, 0);
  lcd_i2c_printf(&manager->lcd_i2c, ">%s %s", selected_sport->name,
                 selected_sport->variation);

  lcd_i2c_set_cursor(&manager->lcd_i2c, 0, 1);
  lcd_i2c_printf(&manager->lcd_i2c, "Time: %03d", current_seconds);
}

// -----------------------------------------------------------------------------
// Helpers - ST7735 drawing
// -----------------------------------------------------------------------------
static void ui_draw_st7735_frame(UiManager *manager) {
  st7735_draw_rect_outline(&manager->st7735, 0, 0, ST7735_WIDTH, ST7735_HEIGHT,
                           ST7735_BLUE);
}

static void ui_draw_st7735_main(UiManager *manager, const sport_config_t *sport,
                                uint16_t seconds) {
  St7735Lcd *lcd = &manager->st7735;

  // Clear background
  st7735_clear(lcd, ST7735_BLACK);

  // Outer frame
  ui_draw_st7735_frame(manager);

  // Sport line (centered, small)
  char line1[32];
  snprintf(line1, sizeof(line1), "%s %s", sport->name, sport->variation);
  st7735_print_center(lcd, UI_ST7735_HEADER_Y, ST7735_WHITE, ST7735_BLACK, 1,
                      line1);

  // Timer line (centered, larger)
  char line2[24];
  snprintf(line2, sizeof(line2), "Time: %03d", seconds);
  st7735_print_center(lcd, UI_ST7735_TIMER_Y, ST7735_WHITE, ST7735_BLACK, 2,
                      line2);
}

static void ui_draw_st7735_sport_selection(UiManager *manager,
                                           const sport_config_t *selected_sport,
                                           uint16_t current_seconds) {
  St7735Lcd *lcd = &manager->st7735;

  st7735_clear(lcd, ST7735_BLACK);

  // Outer frame
  ui_draw_st7735_frame(manager);

  // Selection box (outline only, not filled)
  int box_x = UI_ST7735_MARGIN + 2;
  int box_y = UI_ST7735_SELECTION_BOX_Y;
  int box_w = ST7735_WIDTH - (UI_ST7735_MARGIN + 2) * 2;
  int box_h = UI_ST7735_SELECTION_BOX_H;

  st7735_draw_rect_outline(lcd, box_x, box_y, box_w, box_h, ST7735_GREEN);

  // Highlighted sport line, centered
  char line1[32];
  snprintf(line1, sizeof(line1), ">%s %s", selected_sport->name,
           selected_sport->variation);

  // Put it inside selection box vertically
  int text_y =
      box_y + (box_h / 2) - 4; // roughly vertically centered for 8px font
  st7735_print_center(lcd, text_y, ST7735_GREEN, ST7735_BLACK, 1, line1);

  // Timer below selection
  char line2[24];
  snprintf(line2, sizeof(line2), "Time: %03d", current_seconds);
  st7735_print_center(lcd, UI_ST7735_TIMER_Y + 20, ST7735_WHITE, ST7735_BLACK,
                      1, line2);
}

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
  ui_draw_st7735_frame(manager);

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

  if (!sport) {
    ESP_LOGE(TAG, "Sport config is NULL");
    return;
  }

  if (manager->type == DISPLAY_TYPE_LCD_I2C) {
    ui_draw_i2c_main(manager, sport, seconds);
  } else if (manager->type == DISPLAY_TYPE_ST7735) {
    ui_draw_st7735_main(manager, sport, seconds);
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

  if (!selected_sport) {
    ESP_LOGE(TAG, "Selected sport is NULL");
    return;
  }

  if (manager->type == DISPLAY_TYPE_LCD_I2C) {
    ui_draw_i2c_sport_selection(manager, selected_sport, current_seconds);
  } else if (manager->type == DISPLAY_TYPE_ST7735) {
    ui_draw_st7735_sport_selection(manager, selected_sport, current_seconds);
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

  if (manager->type == DISPLAY_TYPE_LCD_I2C) {
    lcd_i2c_clear(&manager->lcd_i2c);
  } else if (manager->type == DISPLAY_TYPE_ST7735) {
    st7735_clear(&manager->st7735, ST7735_BLACK);
    ui_draw_st7735_frame(manager);
  }
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
    ui_manager_clear(manager);
  }
}
