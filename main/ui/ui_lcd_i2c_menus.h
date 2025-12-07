#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "sport_manager.h"
#include "sport_selector.h"
#include "ui_manager.h"

void ui_draw_i2c_sport_menu(UiManager *m, const sport_group_t *groups,
                            size_t group_count, uint8_t selected_group_idx);
void ui_draw_i2c_variant_menu(UiManager *m, const sport_group_t *group);