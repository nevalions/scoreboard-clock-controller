#include "ui_lcd_i2c_main.h"
#include "ui_helpers.h"
#include "ui_manager.h"
#include <stdio.h>
#include <string.h>

void ui_draw_i2c_main(UiManager *m, const sport_config_t *sport, uint16_t sec) {
  char buf[8];
  ui_format_seconds(buf, sizeof(buf), sec);

  lcd_i2c_clear(&m->lcd_i2c);
  lcd_i2c_set_cursor(&m->lcd_i2c, 0, 0);
  lcd_i2c_printf(&m->lcd_i2c, "%s %s", sport->name, sport->variation);

  lcd_i2c_set_cursor(&m->lcd_i2c, UI_I2C_LINE_TIME_COL, 1);
  lcd_i2c_printf(&m->lcd_i2c, "%s", buf);
}