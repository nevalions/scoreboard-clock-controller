#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "sport_selector.h"
#include "ui_manager.h"

void ui_draw_i2c_main(UiManager *m, const sport_config_t *sport, uint16_t sec);