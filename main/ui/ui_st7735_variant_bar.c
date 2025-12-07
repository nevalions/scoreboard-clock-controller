#include "ui_st7735_variant_bar.h"
#include "ui_helpers.h"
#include "ui_manager.h"
#include "sport_selector.h"
#include <stdio.h>
#include <string.h>

void ui_draw_st7735_variant_bar(UiManager *m, const SportManager *sm) {
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