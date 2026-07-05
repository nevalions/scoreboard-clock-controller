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

void ui_st7735_draw_status(UiManager *m, bool running, bool link_good,
                           uint8_t brightness_pct) {
  St7735Lcd *lcd = &m->st7735;

  // RUN/PAUSE glyph, top-left corner (sport name is centered, so the
  // corners of the header row are free)
  st7735_draw_rect(lcd, UI_ST7735_MARGIN + 2, 2, 36, 8, ST7735_BLACK);
  if (running) {
    st7735_print(lcd, UI_ST7735_MARGIN + 2, 2, ST7735_GREEN, ST7735_BLACK, 1,
                 "RUN");
  } else {
    st7735_print(lcd, UI_ST7735_MARGIN + 2, 2, ST7735_YELLOW, ST7735_BLACK, 1,
                 "PAUSE");
  }

  // TX brightness percentage, left of the link dot (rotary click cycles it)
  char pctbuf[6];
  snprintf(pctbuf, sizeof(pctbuf), "%3u%%", brightness_pct);
  st7735_draw_rect(lcd, ST7735_WIDTH - UI_ST7735_MARGIN - 40, 2, 26, 8,
                   ST7735_BLACK);
  st7735_print(lcd, ST7735_WIDTH - UI_ST7735_MARGIN - 40, 2, ST7735_WHITE,
               ST7735_BLACK, 1, pctbuf);

  // Radio link dot, top-right corner: green = frames airing, red = not
  uint16_t link_color = link_good ? ST7735_GREEN : ST7735_RED;
  st7735_draw_rect(lcd, ST7735_WIDTH - UI_ST7735_MARGIN - 10, 2, 8, 8,
                   link_color);
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