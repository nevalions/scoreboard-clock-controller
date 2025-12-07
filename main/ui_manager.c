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

// Header spacing
#define UI_ST7735_HEADER_Y 10 // better top padding

// Variant bar layout
#define UI_ST7735_VARIANT_BAR_Y 30
#define UI_ST7735_VARIANT_BAR_SPACING 30
#define UI_ST7735_VARIANT_HIGHLIGHT_COLOR ST7735_YELLOW
#define UI_ST7735_VARIANT_NORMAL_COLOR ST7735_WHITE

// Variant list menu layout
#define UI_ST7735_VARIANT_LIST_Y 70
#define UI_ST7735_VARIANT_SPACING 22

// Sport menu layout
#define UI_ST7735_MENU_LIST_Y 60
#define UI_ST7735_LINE_SPACING 10

// -----------------------------------------------------------------------------
// Format seconds
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
// HEADER DOUBLE UNDERLINE (Option E)
// -----------------------------------------------------------------------------
static void ui_draw_st7735_header_underline(St7735Lcd *lcd) {
  // Bar paddings
  const int pad = 12;
  int bar_w = ST7735_WIDTH - (pad * 2);

  // First thin line
  int y1 = UI_ST7735_HEADER_Y + 12;
  // Second thicker line (below by 2px)
  int y2 = y1 + 3;

  st7735_draw_rect(lcd, pad, y1, bar_w, 1, ST7735_WHITE); // thin line
  st7735_draw_rect(lcd, pad, y2, bar_w, 2, ST7735_WHITE); // thick line
}

// -----------------------------------------------------------------------------
// ST7735 Frame
// -----------------------------------------------------------------------------
static void ui_draw_st7735_frame(UiManager *m) {
  st7735_draw_rect_outline(&m->st7735, 0, 0, ST7735_WIDTH, ST7735_HEIGHT,
                           ST7735_BLUE);
}

// -----------------------------------------------------------------------------
// CENTERED OUTLINED VARIANT BAR (with boxes)
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// CENTERED OUTLINED VARIANT BAR (adaptive for 4+ timers)
// -----------------------------------------------------------------------------
static void ui_draw_st7735_variant_bar(UiManager *m, const SportManager *sm) {
  St7735Lcd *lcd = &m->st7735;

  const sport_group_t *group = sport_manager_get_current_group(sm);
  if (!group || group->variant_count == 0)
    return;

  uint8_t active_idx = sport_manager_get_current_variant_index(sm);
  uint8_t count = group->variant_count;

  const int y = UI_ST7735_VARIANT_BAR_Y;

  // ---------------------------------------------------------------------------
  // CASE 1: 1–3 variants  -> big boxes (your original layout)
  // ---------------------------------------------------------------------------
  if (count <= 3) {
    const int box_w = 32;
    const int box_h = 18;
    const int spacing = 12;

    int total_width = count * box_w + (count - 1) * spacing;
    int x = (ST7735_WIDTH - total_width) / 2;
    if (x < 0)
      x = 0;

    for (uint8_t i = 0; i < count; i++) {
      sport_config_t cfg = get_sport_config(group->variants[i]);

      bool active = (i == active_idx);
      uint16_t color = active ? UI_ST7735_VARIANT_HIGHLIGHT_COLOR
                              : UI_ST7735_VARIANT_NORMAL_COLOR;

      // Draw outlined box
      st7735_draw_rect_outline(lcd, x, y, box_w, box_h, color);

      // Timer text as string
      char buf[12];
      snprintf(buf, sizeof(buf), "%u", cfg.play_clock_seconds);

      int text_px = strlen(buf) * 8;
      int text_x = x + (box_w - text_px) / 2;
      int text_y = y + 4;

      if (text_x < 0)
        text_x = 0;

      st7735_print(lcd, text_x, text_y, color, ST7735_BLACK, 1, buf);

      x += box_w + spacing;
    }
    return;
  }

  // ---------------------------------------------------------------------------
  // CASE 2: exactly 4 variants -> slightly smaller boxes & tighter spacing
  // ---------------------------------------------------------------------------
  if (count == 4) {
    const int box_w = 26; // narrower box
    const int box_h = 18;
    const int spacing = 6; // tighter spacing

    int total_width = count * box_w + (count - 1) * spacing;
    int x = (ST7735_WIDTH - total_width) / 2;
    if (x < 0)
      x = 0;

    for (uint8_t i = 0; i < count; i++) {
      sport_config_t cfg = get_sport_config(group->variants[i]);

      bool active = (i == active_idx);
      uint16_t color = active ? UI_ST7735_VARIANT_HIGHLIGHT_COLOR
                              : UI_ST7735_VARIANT_NORMAL_COLOR;

      st7735_draw_rect_outline(lcd, x, y, box_w, box_h, color);

      char buf[12];
      snprintf(buf, sizeof(buf), "%u", cfg.play_clock_seconds);

      int text_px = strlen(buf) * 8;
      int text_x = x + (box_w - text_px) / 2;
      int text_y = y + 4;

      if (text_x < 0)
        text_x = 0;

      st7735_print(lcd, text_x, text_y, color, ST7735_BLACK, 1, buf);

      x += box_w + spacing;
    }
    return;
  }

  // ---------------------------------------------------------------------------
  // CASE 3: 5+ variants -> compact text-only bar (no boxes)
  // ---------------------------------------------------------------------------
  const int spacing_px = 4; // space between values
  int widths[8];            // supports up to 8 variants comfortably
  char texts[8][12];

  if (count > 8)
    count = 8; // hard cap, just in case

  int total_text_width = 0;
  for (uint8_t i = 0; i < count; i++) {
    sport_config_t cfg = get_sport_config(group->variants[i]);
    snprintf(texts[i], sizeof(texts[i]), "%u", cfg.play_clock_seconds);
    widths[i] = strlen(texts[i]) * 8;
    total_text_width += widths[i];
    if (i + 1 < count)
      total_text_width += spacing_px;
  }

  int x = (ST7735_WIDTH - total_text_width) / 2;
  if (x < 0)
    x = 0;

  for (uint8_t i = 0; i < count; i++) {
    bool active = (i == active_idx);
    uint16_t color = active ? UI_ST7735_VARIANT_HIGHLIGHT_COLOR
                            : UI_ST7735_VARIANT_NORMAL_COLOR;

    // If you want a subtle underline for active instead of box:
    // if (active) {
    //   st7735_draw_rect(lcd, x, y + 14, widths[i], 2, color);
    // }

    st7735_print(lcd, x, y + 4, color, ST7735_BLACK, 1, texts[i]);
    x += widths[i] + spacing_px;
  }
}

// -----------------------------------------------------------------------------
// RUNNING SCREEN (Header + underline + variant bar + big timer)
// -----------------------------------------------------------------------------
static void ui_draw_st7735_main(UiManager *m, const sport_config_t *sport,
                                uint16_t sec, const SportManager *sm) {
  St7735Lcd *lcd = &m->st7735;

  char timebuf[8];
  format_seconds(timebuf, sizeof(timebuf), sec);

  st7735_clear(lcd, ST7735_BLACK);
  ui_draw_st7735_frame(m);

  // Header
  st7735_print_center(lcd, UI_ST7735_HEADER_Y, ST7735_WHITE, ST7735_BLACK, 1,
                      sport->name);

  // Double underline (Option E)
  ui_draw_st7735_header_underline(lcd);

  // Variant bar
  ui_draw_st7735_variant_bar(m, sm);

  // Big timer
  st7735_print_center(lcd, 85, ST7735_WHITE, ST7735_BLACK, 4, timebuf);
}

// -----------------------------------------------------------------------------
// FAST TIMER UPDATE (running mode)
// -----------------------------------------------------------------------------
static void ui_st7735_update_time(UiManager *m, const sport_config_t *sport,
                                  uint16_t sec, const SportManager *sm) {
  (void)sport; // header + variant bar unchanged on increments

  char buf[8];
  format_seconds(buf, sizeof(buf), sec);

  St7735Lcd *lcd = &m->st7735;

  // Only clear big timer area (no flicker)
  st7735_draw_rect(lcd, 10, 70, ST7735_WIDTH - 20, 80, ST7735_BLACK);

  st7735_print_center(lcd, 85, ST7735_WHITE, ST7735_BLACK, 4, buf);
}

// -----------------------------------------------------------------------------
// I2C LCD modes (unchanged)
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

// -----------------------------------------------------------------------------
// I2C LCD – SPORT MENU
// -----------------------------------------------------------------------------
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

  sport_config_t cfg = get_sport_config(groups[selected_group_idx].variants[0]);

  lcd_i2c_set_cursor(&m->lcd_i2c, 0, 1);
  lcd_i2c_printf(&m->lcd_i2c, ">%s", cfg.name);
}

// -----------------------------------------------------------------------------
// I2C LCD – VARIANT MENU
// -----------------------------------------------------------------------------
static void ui_draw_i2c_variant_menu(UiManager *m, const sport_group_t *group) {

  if (!group || group->variant_count == 0)
    return;

  sport_config_t base_cfg = get_sport_config(group->variants[0]);

  lcd_i2c_clear(&m->lcd_i2c);
  lcd_i2c_set_cursor(&m->lcd_i2c, 0, 0);
  lcd_i2c_printf(&m->lcd_i2c, "%s", base_cfg.name);

  lcd_i2c_set_cursor(&m->lcd_i2c, 0, 1);

  for (uint8_t i = 0; i < group->variant_count; i++) {
    sport_config_t cfg = get_sport_config(group->variants[i]);
    lcd_i2c_printf(&m->lcd_i2c, "%u", cfg.play_clock_seconds);

    if (i + 1 < group->variant_count)
      lcd_i2c_printf(&m->lcd_i2c, " ");
  }
}

// -----------------------------------------------------------------------------
// SPORT MENU (ST7735) — unchanged
// -----------------------------------------------------------------------------
static void ui_draw_st7735_sport_menu(UiManager *m, const sport_group_t *groups,
                                      size_t group_count,
                                      uint8_t selected_idx) {
  St7735Lcd *lcd = &m->st7735;

  st7735_clear(lcd, ST7735_BLACK);
  ui_draw_st7735_frame(m);

  st7735_print_center(lcd, UI_ST7735_HEADER_Y, ST7735_YELLOW, ST7735_BLACK, 1,
                      "SELECT SPORT");

  ui_draw_st7735_header_underline(lcd);

  int y = UI_ST7735_MENU_LIST_Y;

  for (size_t i = 0; i < group_count; i++) {
    sport_config_t cfg = get_sport_config(groups[i].variants[0]);

    char line[32];
    snprintf(line, sizeof(line), "%c %s", (i == selected_idx) ? '>' : ' ',
             cfg.name);

    st7735_print(lcd, UI_ST7735_MARGIN + 4, y, ST7735_WHITE, ST7735_BLACK, 1,
                 line);

    y += UI_ST7735_LINE_SPACING;
  }
}

// -----------------------------------------------------------------------------
// VARIANT MENU (ST7735)
// -----------------------------------------------------------------------------
static void ui_draw_st7735_variant_menu(UiManager *m,
                                        const sport_group_t *group) {
  St7735Lcd *lcd = &m->st7735;

  st7735_clear(lcd, ST7735_BLACK);
  ui_draw_st7735_frame(m);

  if (!group || group->variant_count == 0) {
    st7735_print_center(lcd, UI_ST7735_HEADER_Y, ST7735_WHITE, ST7735_BLACK, 1,
                        "NO VARIANTS");
    ui_draw_st7735_header_underline(lcd);
    return;
  }

  sport_config_t cfg0 = get_sport_config(group->variants[0]);

  st7735_print_center(lcd, UI_ST7735_HEADER_Y, ST7735_YELLOW, ST7735_BLACK, 1,
                      cfg0.name);

  ui_draw_st7735_header_underline(lcd);

  int y = UI_ST7735_VARIANT_LIST_Y;

  for (uint8_t i = 0; i < group->variant_count; i++) {
    sport_config_t cfg = get_sport_config(group->variants[i]);
    char line[32];
    snprintf(line, sizeof(line), "(%u) %u", i + 1, cfg.play_clock_seconds);

    st7735_print_center(lcd, y, ST7735_WHITE, ST7735_BLACK, 2, line);

    y += UI_ST7735_VARIANT_SPACING;
  }
}

// -----------------------------------------------------------------------------
// PUBLIC API
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

void ui_st7735_update_sport_menu_selection(UiManager *m,
                                           const sport_group_t *groups,
                                           size_t group_count,
                                           uint8_t selected_group_idx) {
  if (!m || !m->initialized)
    return;

  St7735Lcd *lcd = &m->st7735;
  int y = UI_ST7735_MENU_LIST_Y;

  for (size_t i = 0; i < group_count; i++) {
    char marker = (i == selected_group_idx) ? '>' : ' ';

    st7735_draw_rect(lcd, UI_ST7735_MARGIN + 2, y, 10, 10, ST7735_BLACK);

    char str[2] = {marker, '\0'};
    st7735_print(lcd, UI_ST7735_MARGIN + 2, y, ST7735_WHITE, ST7735_BLACK, 1,
                 str);

    y += UI_ST7735_LINE_SPACING;
  }
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
