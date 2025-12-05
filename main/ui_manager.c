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
#define UI_ST7735_HEADER_Y 4
#define UI_ST7735_TIMER_Y 40

// Sport menu layout
#define UI_ST7735_LOGO_Y 18
#define UI_ST7735_MENU_LIST_Y 60
#define UI_ST7735_LINE_SPACING 10

// Variant list layout
#define UI_ST7735_VARIANT_LIST_Y 70
#define UI_ST7735_VARIANT_SPACING 10

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
// I2C LCD drawing (simple versions)
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

static void ui_draw_i2c_sport_menu(UiManager *m, const sport_group_t *groups,
                                   size_t group_count,
                                   uint8_t selected_group_idx) {
  lcd_i2c_clear(&m->lcd_i2c);
  lcd_i2c_set_cursor(&m->lcd_i2c, 0, 0);
  lcd_i2c_printf(&m->lcd_i2c, "SELECT SPORT");

  if (group_count == 0)
    return;
  if (selected_group_idx >= group_count)
    selected_group_idx = 0;

  // Get sport name from first variant in group
  sport_config_t cfg = get_sport_config(groups[selected_group_idx].variants[0]);

  lcd_i2c_set_cursor(&m->lcd_i2c, 0, 1);
  lcd_i2c_printf(&m->lcd_i2c, ">%s", cfg.name);
}

static void ui_draw_i2c_variant_menu(UiManager *m, const sport_group_t *group) {
  if (!group || group->variant_count == 0)
    return;

  // Base sport name from first variant
  sport_config_t base_cfg = get_sport_config(group->variants[0]);

  lcd_i2c_clear(&m->lcd_i2c);
  lcd_i2c_set_cursor(&m->lcd_i2c, 0, 0);
  lcd_i2c_printf(&m->lcd_i2c, "%s", base_cfg.name);

  // On 2nd line, list playclocks as "24 30" etc.
  lcd_i2c_set_cursor(&m->lcd_i2c, 0, 1);
  for (uint8_t i = 0; i < group->variant_count; i++) {
    sport_config_t cfg = get_sport_config(group->variants[i]);
    lcd_i2c_printf(&m->lcd_i2c, "%u", cfg.play_clock_seconds);
    if (i + 1 < group->variant_count) {
      lcd_i2c_printf(&m->lcd_i2c, " ");
    }
  }
}

// -----------------------------------------------------------------------------
// ST7735 drawing helpers
// -----------------------------------------------------------------------------
static void ui_draw_st7735_frame(UiManager *m) {
  st7735_draw_rect_outline(&m->st7735, 0, 0, ST7735_WIDTH, ST7735_HEIGHT,
                           ST7735_BLUE);
}

// Running mode: only header (sport) + big timer
static void ui_draw_st7735_main(UiManager *m, const sport_config_t *sport,
                                uint16_t sec) {

  char timebuf[8];
  format_seconds(timebuf, sizeof(timebuf), sec);

  St7735Lcd *lcd = &m->st7735;

  st7735_clear(lcd, ST7735_BLACK);
  ui_draw_st7735_frame(m);

  // Header: sport name
  st7735_print_center(lcd, UI_ST7735_HEADER_Y, ST7735_WHITE, ST7735_BLACK, 1,
                      sport->name);

  // Timer (big)
  st7735_print_center(lcd, UI_ST7735_TIMER_Y, ST7735_WHITE, ST7735_BLACK, 2,
                      timebuf);
}

// Level 1: Sport selection menu (with '>' cursor)
static void ui_draw_st7735_sport_menu(UiManager *m, const sport_group_t *groups,
                                      size_t group_count,
                                      uint8_t selected_group_idx) {

  St7735Lcd *lcd = &m->st7735;

  st7735_clear(lcd, ST7735_BLACK);
  ui_draw_st7735_frame(m);

  if (group_count == 0) {
    st7735_print_center(lcd, UI_ST7735_HEADER_Y, ST7735_WHITE, ST7735_BLACK, 1,
                        "NO SPORTS");
    return;
  }

  if (selected_group_idx >= group_count)
    selected_group_idx = 0;

  // Header
  st7735_print_center(lcd, UI_ST7735_HEADER_Y, ST7735_YELLOW, ST7735_BLACK, 1,
                      "SELECT SPORT");

  // Logo (3 lines) from selected group
  const sport_group_t *g = &groups[selected_group_idx];
  int logo_y = UI_ST7735_LOGO_Y;
  for (int i = 0; i < 3; i++) {
    if (g->logo[i]) {
      st7735_print_center(lcd, logo_y, ST7735_CYAN, ST7735_BLACK, 1,
                          g->logo[i]);
      logo_y += 10;
    }
  }

  // List of sport names: each from first variant's config
  int y = UI_ST7735_MENU_LIST_Y;
  for (size_t i = 0; i < group_count; i++) {
    sport_config_t cfg = get_sport_config(groups[i].variants[0]);
    char line[32];
    snprintf(line, sizeof(line), "%c %s", (i == selected_group_idx) ? '>' : ' ',
             cfg.name);
    st7735_print(lcd, UI_ST7735_MARGIN + 2, y, ST7735_WHITE, ST7735_BLACK, 1,
                 line);
    y += UI_ST7735_LINE_SPACING;
  }
}

// Level 2: Variant menu (no '>', just show all playclock values)
static void ui_draw_st7735_variant_menu(UiManager *m,
                                        const sport_group_t *group) {

  St7735Lcd *lcd = &m->st7735;

  st7735_clear(lcd, ST7735_BLACK);
  ui_draw_st7735_frame(m);

  if (!group || group->variant_count == 0) {
    st7735_print_center(lcd, UI_ST7735_HEADER_Y, ST7735_WHITE, ST7735_BLACK, 1,
                        "NO VARIANTS");
    return;
  }

  // Header: sport name from first variant
  sport_config_t base_cfg = get_sport_config(group->variants[0]);
  st7735_print_center(lcd, UI_ST7735_HEADER_Y, ST7735_YELLOW, ST7735_BLACK, 1,
                      base_cfg.name);

  // Logo
  int logo_y = UI_ST7735_LOGO_Y;
  for (int i = 0; i < 3; i++) {
    if (group->logo[i]) {
      st7735_print_center(lcd, logo_y, ST7735_CYAN, ST7735_BLACK, 1,
                          group->logo[i]);
      logo_y += 10;
    }
  }

  // Variants list: numeric seconds, no '>'
  int y = UI_ST7735_VARIANT_LIST_Y;
  for (uint8_t i = 0; i < group->variant_count; i++) {
    sport_config_t cfg = get_sport_config(group->variants[i]);
    char line[16];
    snprintf(line, sizeof(line), "%u", cfg.play_clock_seconds);
    st7735_print_center(lcd, y, ST7735_WHITE, ST7735_BLACK, 1, line);
    y += UI_ST7735_VARIANT_SPACING;
  }
}

// -----------------------------------------------------------------------------
// ST7735: fast timer update (running-only)
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
// Public API: timer update (running mode only)
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
// Full redraw API (running mode)
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

// -----------------------------------------------------------------------------
// New menu APIs
// -----------------------------------------------------------------------------
void ui_manager_show_sport_menu(UiManager *m, const sport_group_t *groups,
                                size_t group_count,
                                uint8_t selected_group_idx) {
  if (!m || !m->initialized)
    return;

  if (m->type == DISPLAY_TYPE_LCD_I2C)
    ui_draw_i2c_sport_menu(m, groups, group_count, selected_group_idx);
  else
    ui_draw_st7735_sport_menu(m, groups, group_count, selected_group_idx);
}

void ui_manager_show_variant_menu(UiManager *m, const sport_group_t *group) {
  if (!m || !m->initialized)
    return;

  if (m->type == DISPLAY_TYPE_LCD_I2C)
    ui_draw_i2c_variant_menu(m, group);
  else
    ui_draw_st7735_variant_menu(m, group);
}

void ui_st7735_update_sport_menu_selection(UiManager *m,
                                           const sport_group_t *groups,
                                           size_t group_count,
                                           uint8_t selected_group_idx) {
  if (!m || !m->initialized)
    return;
  St7735Lcd *lcd = &m->st7735;

  // ------- Update ONLY the logo (clear old, draw new) ----------
  // Coordinates MUST match full-menu layout
  int logo_y = UI_ST7735_LOGO_Y;

  // Clear logo area (approx box)
  st7735_draw_rect(lcd, 0, logo_y - 2, ST7735_WIDTH, 40, ST7735_BLACK);

  // Draw new logo
  const sport_group_t *g = &groups[selected_group_idx];
  for (int i = 0; i < 3; i++) {
    if (g->logo[i]) {
      st7735_print_center(lcd, logo_y, ST7735_CYAN, ST7735_BLACK, 1,
                          g->logo[i]);
      logo_y += 10;
    }
  }

  // ------- Update ONLY the '>' marks in sport list ----------
  int y = UI_ST7735_MENU_LIST_Y;

  for (size_t i = 0; i < group_count; i++) {
    char prefix = (i == selected_group_idx) ? '>' : ' ';
    char line[32];
    sport_config_t cfg = get_sport_config(groups[i].variants[0]);

    snprintf(line, sizeof(line), "%c %s", prefix, cfg.name);

    // Clear text area for this line (only prefix)
    st7735_draw_rect(lcd, UI_ST7735_MARGIN + 2, y, 10, 10, ST7735_BLACK);

    // Draw updated prefix only ("> " or "  ")
    char prefix_str[3] = {prefix, '\0'};
    st7735_print(lcd, UI_ST7735_MARGIN + 2, y, ST7735_WHITE, ST7735_BLACK, 1,
                 prefix_str);

    y += UI_ST7735_LINE_SPACING;
  }
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
