#include <stdint.h>
#include <stdbool.h>
#include "esp_log.h"
#include "sport_manager.h"
#include "../../sport_selector/include/sport_selector.h"

static const char *TAG = "SPORT_MGR";

void sport_manager_init(SportManager *manager) {
    manager->current_sport = get_sport_config(SPORT_BASKETBALL_24_SEC);
    manager->selected_sport_type = SPORT_BASKETBALL_24_SEC;
}

sport_type_t sport_manager_get_next(sport_type_t current) {
    switch (current) {
    case SPORT_BASKETBALL_24_SEC:
        return SPORT_BASKETBALL_30_SEC;
    case SPORT_BASKETBALL_30_SEC:
        return SPORT_FOOTBALL_40_SEC;
    case SPORT_FOOTBALL_40_SEC:
        return SPORT_FOOTBALL_25_SEC;
    case SPORT_FOOTBALL_25_SEC:
        return SPORT_BASEBALL_15_SEC;
    case SPORT_BASEBALL_15_SEC:
        return SPORT_BASEBALL_20_SEC;
    case SPORT_BASEBALL_20_SEC:
        return SPORT_BASEBALL_14_SEC;
    case SPORT_BASEBALL_14_SEC:
        return SPORT_BASEBALL_19_SEC;
    case SPORT_BASEBALL_19_SEC:
        return SPORT_VOLLEYBALL_8_SEC;
    case SPORT_VOLLEYBALL_8_SEC:
        return SPORT_LACROSSE_30_SEC;
    case SPORT_LACROSSE_30_SEC:
        return SPORT_BASKETBALL_24_SEC;
    default:
        return SPORT_BASKETBALL_24_SEC;
    }
}

sport_type_t sport_manager_get_prev(sport_type_t current) {
    switch (current) {
    case SPORT_BASKETBALL_24_SEC:
        return SPORT_LACROSSE_30_SEC;
    case SPORT_BASKETBALL_30_SEC:
        return SPORT_BASKETBALL_24_SEC;
    case SPORT_FOOTBALL_40_SEC:
        return SPORT_BASKETBALL_30_SEC;
    case SPORT_FOOTBALL_25_SEC:
        return SPORT_FOOTBALL_40_SEC;
    case SPORT_BASEBALL_15_SEC:
        return SPORT_FOOTBALL_25_SEC;
    case SPORT_BASEBALL_20_SEC:
        return SPORT_BASEBALL_15_SEC;
    case SPORT_BASEBALL_14_SEC:
        return SPORT_BASEBALL_20_SEC;
    case SPORT_BASEBALL_19_SEC:
        return SPORT_BASEBALL_14_SEC;
    case SPORT_VOLLEYBALL_8_SEC:
        return SPORT_BASEBALL_19_SEC;
    case SPORT_LACROSSE_30_SEC:
        return SPORT_VOLLEYBALL_8_SEC;
    default:
        return SPORT_BASKETBALL_24_SEC;
    }
}

void sport_manager_set_sport(SportManager *manager, sport_type_t sport_type) {
    manager->current_sport = get_sport_config(sport_type);
    manager->selected_sport_type = sport_type;
    
    ESP_LOGI(TAG, "Sport set to: %s %s (%d seconds)", 
             manager->current_sport.name,
             manager->current_sport.variation, 
             manager->current_sport.play_clock_seconds);
}

sport_config_t sport_manager_get_current_sport(const SportManager *manager) {
    return manager->current_sport;
}

sport_type_t sport_manager_get_selected_type(const SportManager *manager) {
    return manager->selected_sport_type;
}

void sport_manager_set_selected_type(SportManager *manager, sport_type_t sport_type) {
    manager->selected_sport_type = sport_type;
}