#include "ui_st7735_main.h"
#include "ui_helpers.h"
#include "ui_st7735_variant_bar.h"
#include "ui_manager.h"
#include <stdio.h>

void ui_draw_st7735_main(UiManager *m, const sport_config_t *sport,
                         uint16_t sec, const SportManager *sm) {
  St7735Lcd *lcd = &m->st7735;

  char timebuf[8];
  ui_format_seconds(timebuf, sizeof(timebuf), sec);

  st7735_clear(lcd, ST7735_BLACK);
  ui_draw_st7735_frame(m);

  // Header
  ui_st7735_print_center(lcd, UI_ST7735_HEADER_Y, ST7735_WHITE, ST7735_BLACK, 1,
                          sport->name);

  // Double underline (Option E)
  ui_draw_st7735_header_underline(lcd);

  // Variant bar
  ui_draw_st7735_variant_bar(m, sm);

  // Big timer
  ui_st7735_print_center(lcd, 85, ST7735_WHITE, ST7735_BLACK, 4, timebuf);
}

void ui_st7735_update_time(UiManager *m, const sport_config_t *sport,
                          uint16_t sec, const SportManager *sm) {
  (void)sport; // header + variant bar unchanged on increments
  (void)sm;    // sport manager not needed for time-only update

  char buf[8];
  ui_format_seconds(buf, sizeof(buf), sec);

  St7735Lcd *lcd = &m->st7735;

  // Only clear big timer area (no flicker)
  st7735_draw_rect(lcd, 10, 70, ST7735_WIDTH - 20, 80, ST7735_BLACK);

  ui_st7735_print_center(lcd, 85, ST7735_WHITE, ST7735_BLACK, 4, buf);
}