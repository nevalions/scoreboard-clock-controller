#include "ui_lcd_i2c_menus.h"
#include "ui_helpers.h"
#include "ui_manager.h"
#include "sport_selector.h"
#include <stdio.h>
#include <string.h>

void ui_draw_i2c_sport_menu(UiManager *m, const sport_group_t *groups,
                            size_t group_count, uint8_t selected_group_idx) {

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

void ui_draw_i2c_variant_menu(UiManager *m, const sport_group_t *group) {

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