#include "ui_helpers.h"
#include "ui_manager.h"
#include <string.h>

static const char *TAG = "UI_HELPERS";

void ui_format_seconds(char *out, size_t size, uint16_t sec) {
  if (sec < 10)
    snprintf(out, size, "0%d", sec);
  else
    snprintf(out, size, "%d", sec);
}

void ui_st7735_print_center(St7735Lcd *lcd, int y, uint16_t fg, uint16_t bg,
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

void ui_draw_st7735_header_underline(St7735Lcd *lcd) {
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

void ui_draw_st7735_frame(UiManager *m) {
  st7735_draw_rect_outline(&m->st7735, 0, 0, ST7735_WIDTH, ST7735_HEIGHT,
                           ST7735_BLUE);
}