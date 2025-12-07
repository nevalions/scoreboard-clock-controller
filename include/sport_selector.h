#ifndef SPORT_SELECTOR_H
#define SPORT_SELECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include "colors.h"

typedef enum {
    SPORT_PLAYCLOCK_NULL = 0,
    SPORT_BASKETBALL_24_SEC,
    SPORT_BASKETBALL_30_SEC,
    SPORT_FOOTBALL_40_SEC,
    SPORT_FOOTBALL_25_SEC,
    SPORT_BASEBALL_15_SEC,
    SPORT_BASEBALL_20_SEC,
    SPORT_BASEBALL_14_SEC,
    SPORT_BASEBALL_19_SEC,
    SPORT_VOLLEYBALL_8_SEC,
    SPORT_LACROSSE_30_SEC,
    SPORT_CUSTOM_ONE_RESET,
    SPORT_CUSTOM_ONE_PAUSE,
    SPORT_CUSTOM_ONE_MIXED,
    SPORT_CUSTOM_TWO_RESET,
    SPORT_CUSTOM_TWO_PAUSE,
    SPORT_CUSTOM_TWO_MIXED,
    SPORT_COUNT_ONE,
    SPORT_COUNT_TWO
} sport_type_t;

typedef enum {
    PLAYCLOCK_BEHAVIOR_RESET = 0,
    PLAYCLOCK_BEHAVIOR_PAUSE,
    PLAYCLOCK_BEHAVIOR_MIXED
} playclock_behavior_t;

typedef struct {
    sport_type_t sport;
    uint8_t play_clock_seconds;
    const char* name;
    const char* variation;
    playclock_behavior_t behavior;
    color_scheme_t color_scheme;
} sport_config_t;

sport_config_t get_sport_config(sport_type_t sport);
sport_config_t get_custom_config(uint8_t count, playclock_behavior_t behavior);
sport_config_t get_basketball_config(uint8_t seconds);
sport_config_t get_football_config(uint8_t seconds);
sport_config_t get_baseball_config(uint8_t seconds);
color_scheme_t get_sport_color_scheme(sport_type_t sport);

#endif