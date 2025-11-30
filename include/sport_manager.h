#pragma once

#include "../../sport_selector/include/sport_selector.h"

typedef struct {
    sport_config_t current_sport;
    sport_type_t selected_sport_type;
} SportManager;

void sport_manager_init(SportManager *manager);
sport_type_t sport_manager_get_next(sport_type_t current);
sport_type_t sport_manager_get_prev(sport_type_t current);
void sport_manager_set_sport(SportManager *manager, sport_type_t sport_type);
sport_config_t sport_manager_get_current_sport(const SportManager *manager);
sport_type_t sport_manager_get_selected_type(const SportManager *manager);
void sport_manager_set_selected_type(SportManager *manager, sport_type_t sport_type);