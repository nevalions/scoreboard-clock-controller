#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "st7735_lcd.h"
#include "sport_manager.h"
#include "ui_manager.h"

void ui_draw_st7735_variant_bar(UiManager *m, const SportManager *sm);