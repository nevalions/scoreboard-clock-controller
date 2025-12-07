#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "sport_manager.h"
#include "sport_selector.h"
#include "ui_manager.h"

void ui_draw_st7735_main(UiManager *m, const sport_config_t *sport,
                         uint16_t sec, const SportManager *sm);
void ui_st7735_update_time(UiManager *m, const sport_config_t *sport,
                          uint16_t sec, const SportManager *sm);