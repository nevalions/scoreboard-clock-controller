#include "ui_st7735_menus.h"
#include "ui_helpers.h"
#include "ui_manager.h"
#include "sport_selector.h"
#include <stdio.h>
#include <string.h>

void ui_draw_st7735_sport_menu(UiManager *m, const sport_group_t *groups,
                               size_t group_count, uint8_t selected_idx) {
  St7735Lcd *lcd = &m->st7735;

  st7735_clear(lcd, ST7735_BLACK);
  ui_draw_st7735_frame(m);

  ui_st7735_print_center(lcd, UI_ST7735_HEADER_Y, ST7735_YELLOW, ST7735_BLACK, 1,
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

void ui_draw_st7735_variant_menu(UiManager *m, const sport_group_t *group) {
  St7735Lcd *lcd = &m->st7735;

  st7735_clear(lcd, ST7735_BLACK);
  ui_draw_st7735_frame(m);

  if (!group || group->variant_count == 0) {
    ui_st7735_print_center(lcd, UI_ST7735_HEADER_Y, ST7735_WHITE, ST7735_BLACK, 1,
                            "NO VARIANTS");
    ui_draw_st7735_header_underline(lcd);
    return;
  }

  sport_config_t cfg0 = get_sport_config(group->variants[0]);

  ui_st7735_print_center(lcd, UI_ST7735_HEADER_Y, ST7735_YELLOW, ST7735_BLACK, 1,
                          cfg0.name);

  ui_draw_st7735_header_underline(lcd);

  int y = UI_ST7735_VARIANT_LIST_Y;

  for (uint8_t i = 0; i < group->variant_count; i++) {
    sport_config_t cfg = get_sport_config(group->variants[i]);
    char line[32];
    snprintf(line, sizeof(line), "(%u) %u", i + 1, cfg.play_clock_seconds);

    ui_st7735_print_center(lcd, y, ST7735_WHITE, ST7735_BLACK, 2, line);

    y += UI_ST7735_VARIANT_SPACING;
  }
}

void ui_st7735_update_sport_menu_selection(UiManager *m,
                                           const sport_group_t *groups,
                                           size_t group_count,
                                           uint8_t selected_group_idx) {
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