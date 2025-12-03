
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
// Layout constants
// -----------------------------------------------------------------------------
#define UI_I2C_LINE_TIME_COL 0
#define UI_ST7735_MARGIN 2
#define UI_ST7735_HEADER_Y 8
#define UI_ST7735_TIMER_Y 40
#define UI_ST7735_SELECTION_BOX_Y 22
#define UI_ST7735_SELECTION_BOX_H 28

// -----------------------------------------------------------------------------
// Format scoreboard seconds:
//  0–9   → "0X"
//  >=10  → "X"
// -----------------------------------------------------------------------------
static void format_seconds(char *out, size_t size, uint16_t sec) {
  if (sec < 10)
    snprintf(out, size, "0%d", sec);
  else
    snprintf(out, size, "%d", sec);
}

// -----------------------------------------------------------------------------
// Center text helper
// -----------------------------------------------------------------------------
static void st7735_print_center(St7735Lcd *lcd, int y, uint16_t fg, uint16_t bg,
                                uint8_t size, const char *text) {

  if (!lcd || !lcd->initialized || !text)
    return;

  int len = strlen(text);
  if (len <= 0)
    return;

  int text_width = len * 8 * size;
  int x = (lcd->width - text_width) / 2;
  if (x < 0)
    x = 0;

  st7735_print(lcd, x, y, fg, bg, size, text);
}

// -----------------------------------------------------------------------------
// I2C LCD drawing
// -----------------------------------------------------------------------------
static void ui_draw_i2c_main(UiManager *m, const sport_config_t *sport,
                             uint16_t sec) {
  char buf[8];
  format_seconds(buf, sizeof(buf), sec);

  lcd_i2c_clear(&m->lcd_i2c);
  lcd_i2c_set_cursor(&m->lcd_i2c, 0, 0);
  lcd_i2c_printf(&m->lcd_i2c, "%s %s", sport->name, sport->variation);

  lcd_i2c_set_cursor(&m->lcd_i2c, UI_I2C_LINE_TIME_COL, 1);
  lcd_i2c_printf(&m->lcd_i2c, "%s", buf);
}

static void ui_draw_i2c_sport_selection(UiManager *m, const sport_config_t *s,
                                        uint16_t sec) {
  char buf[8];
  format_seconds(buf, sizeof(buf), sec);

  lcd_i2c_clear(&m->lcd_i2c);
  lcd_i2c_set_cursor(&m->lcd_i2c, 0, 0);
  lcd_i2c_printf(&m->lcd_i2c, ">%s %s", s->name, s->variation);

  lcd_i2c_set_cursor(&m->lcd_i2c, 0, 1);
  lcd_i2c_printf(&m->lcd_i2c, "%s", buf);
}

// -----------------------------------------------------------------------------
// ST7735 drawing helpers
// -----------------------------------------------------------------------------
static void ui_draw_st7735_frame(UiManager *m) {
  st7735_draw_rect_outline(&m->st7735, 0, 0, ST7735_WIDTH, ST7735_HEIGHT,
                           ST7735_BLUE);
}

static void ui_draw_st7735_main(UiManager *m, const sport_config_t *sport,
                                uint16_t sec) {

  char timebuf[8];
  format_seconds(timebuf, sizeof(timebuf), sec);

  St7735Lcd *lcd = &m->st7735;

  st7735_clear(lcd, ST7735_BLACK);
  ui_draw_st7735_frame(m);

  // Header
  char line1[32];
  snprintf(line1, sizeof(line1), "%s %s", sport->name, sport->variation);
  st7735_print_center(lcd, UI_ST7735_HEADER_Y, ST7735_WHITE, ST7735_BLACK, 1,
                      line1);

  // Timer
  st7735_print_center(lcd, UI_ST7735_TIMER_Y, ST7735_WHITE, ST7735_BLACK, 2,
                      timebuf);
}

static void ui_draw_st7735_sport_selection(UiManager *m,
                                           const sport_config_t *s,
                                           uint16_t sec) {

  char timebuf[8];
  format_seconds(timebuf, sizeof(timebuf), sec);

  St7735Lcd *lcd = &m->st7735;

  st7735_clear(lcd, ST7735_BLACK);
  ui_draw_st7735_frame(m);

  // Box
  int box_x = UI_ST7735_MARGIN + 2;
  int box_y = UI_ST7735_SELECTION_BOX_Y;
  int box_w = ST7735_WIDTH - (UI_ST7735_MARGIN + 2) * 2;
  int box_h = UI_ST7735_SELECTION_BOX_H;

  st7735_draw_rect_outline(lcd, box_x, box_y, box_w, box_h, ST7735_GREEN);

  // Sport name
  char line1[32];
  snprintf(line1, sizeof(line1), ">%s %s", s->name, s->variation);
  int text_y = box_y + (box_h / 2) - 4;
  st7735_print_center(lcd, text_y, ST7735_GREEN, ST7735_BLACK, 1, line1);

  // Timer below
  st7735_print_center(lcd, UI_ST7735_TIMER_Y + 20, ST7735_WHITE, ST7735_BLACK,
                      1, timebuf);
}

// -----------------------------------------------------------------------------
// ST7735: fast timer update
// -----------------------------------------------------------------------------
static void ui_st7735_update_time(UiManager *m, const sport_config_t *sport,
                                  uint16_t sec) {
  (void)sport;

  char timebuf[8];
  format_seconds(timebuf, sizeof(timebuf), sec);

  St7735Lcd *lcd = &m->st7735;

  int y = UI_ST7735_TIMER_Y;
  int h = 32;

  // Clear timer area only
  st7735_draw_rect(lcd, 0, y - 2, ST7735_WIDTH, h, ST7735_BLACK);

  // Redraw timer only
  st7735_print_center(lcd, y, ST7735_WHITE, ST7735_BLACK, 2, timebuf);
}

// -----------------------------------------------------------------------------
// Public API: timer update
// -----------------------------------------------------------------------------
void ui_manager_update_time(UiManager *m, const sport_config_t *sport,
                            uint16_t sec) {
  if (!m || !m->initialized)
    return;

  if (m->type == DISPLAY_TYPE_ST7735)
    ui_st7735_update_time(m, sport, sec);
  else
    ui_draw_i2c_main(m, sport, sec);
}

// -----------------------------------------------------------------------------
// Init functions
// -----------------------------------------------------------------------------
void ui_manager_init_lcd_i2c(UiManager *m, uint8_t addr, gpio_num_t sda,
                             gpio_num_t scl) {

  ESP_LOGI(TAG, "Init I2C LCD 0x%02X", addr);
  m->type = DISPLAY_TYPE_LCD_I2C;

  if (!lcd_i2c_begin(&m->lcd_i2c, addr, sda, scl)) {
    ESP_LOGE(TAG, "I2C LCD init failed");
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

// -----------------------------------------------------------------------------
// Full redraw API
// -----------------------------------------------------------------------------
void ui_manager_update_display(UiManager *m, const sport_config_t *sport,
                               uint16_t sec) {
  if (!m || !m->initialized || !sport)
    return;

  if (m->type == DISPLAY_TYPE_LCD_I2C)
    ui_draw_i2c_main(m, sport, sec);
  else
    ui_draw_st7735_main(m, sport, sec);
}

void ui_manager_show_sport_selection(UiManager *m, const sport_config_t *s,
                                     uint16_t sec) {
  if (!m || !m->initialized || !s)
    return;

  if (m->type == DISPLAY_TYPE_LCD_I2C)
    ui_draw_i2c_sport_selection(m, s, sec);
  else
    ui_draw_st7735_sport_selection(m, s, sec);
}

// -----------------------------------------------------------------------------
// Clear screen
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Test patterns
// -----------------------------------------------------------------------------
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
