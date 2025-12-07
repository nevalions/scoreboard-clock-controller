#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "sport_manager.h"
#include "sport_selector.h"
#include "ui_manager.h"

void ui_draw_st7735_sport_menu(UiManager *m, const sport_group_t *groups,
                               size_t group_count, uint8_t selected_idx);
void ui_draw_st7735_variant_menu(UiManager *m, const sport_group_t *group);
void ui_st7735_update_sport_menu_selection(UiManager *m,
                                           const sport_group_t *groups,
                                           size_t group_count,
                                           uint8_t selected_group_idx);