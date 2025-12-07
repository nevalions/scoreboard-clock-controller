#include "../include/sport_selector.h"
#include "../include/colors.h"

// Generate basketball configuration for 24 or 30 second shot clocks
sport_config_t get_basketball_config(uint8_t seconds) {
    sport_config_t config = {0};
    config.sport = (seconds == 24) ? SPORT_BASKETBALL_24_SEC : SPORT_BASKETBALL_30_SEC;
    config.play_clock_seconds = seconds;
    config.name = "Basketball";
    config.variation = (seconds == 24) ? "24 seconds" : "30 seconds";
    config.behavior = PLAYCLOCK_BEHAVIOR_MIXED;
    config.color_scheme = COLOR_SCHEME_BASKETBALL;
    return config;
}

// Generate football configuration for 40 or 25 second play clocks
sport_config_t get_football_config(uint8_t seconds) {
    sport_config_t config = {0};
    config.sport = (seconds == 40) ? SPORT_FOOTBALL_40_SEC : SPORT_FOOTBALL_25_SEC;
    config.play_clock_seconds = seconds;
    config.name = "Football";
    config.variation = (seconds == 40) ? "40 seconds" : "25 seconds";
    config.behavior = PLAYCLOCK_BEHAVIOR_RESET;
    config.color_scheme = COLOR_SCHEME_FOOTBALL;
    return config;
}

// Generate baseball configuration for various pitch clock durations (14, 15, 19, 20 seconds)
sport_config_t get_baseball_config(uint8_t seconds) {
    sport_config_t config = {0};
    static const struct { uint8_t sec; sport_type_t type; const char* var; } baseball_map[] = {
        {15, SPORT_BASEBALL_15_SEC, "15 seconds"},
        {20, SPORT_BASEBALL_20_SEC, "20 seconds"},
        {14, SPORT_BASEBALL_14_SEC, "14 seconds"},
        {19, SPORT_BASEBALL_19_SEC, "19 seconds"}
    };
    
    // Search through baseball duration mappings to find matching seconds value
    for (int i = 0; i < 4; i++) {
        if (baseball_map[i].sec == seconds) {
            config.sport = baseball_map[i].type;
            config.play_clock_seconds = seconds;
            config.name = "Baseball";
            config.variation = baseball_map[i].var;
            config.behavior = PLAYCLOCK_BEHAVIOR_RESET;
            config.color_scheme = COLOR_SCHEME_BASEBALL;
            break;  // Exit loop once match is found
        }
    }
    
    return config;
}

// Get color scheme for a given sport type
color_scheme_t get_sport_color_scheme(sport_type_t sport) {
    switch (sport) {
        case SPORT_BASKETBALL_24_SEC:
        case SPORT_BASKETBALL_30_SEC:
            return COLOR_SCHEME_BASKETBALL;
            
        case SPORT_FOOTBALL_40_SEC:
        case SPORT_FOOTBALL_25_SEC:
            return COLOR_SCHEME_FOOTBALL;
            
        case SPORT_BASEBALL_15_SEC:
        case SPORT_BASEBALL_20_SEC:
        case SPORT_BASEBALL_14_SEC:
        case SPORT_BASEBALL_19_SEC:
            return COLOR_SCHEME_BASEBALL;
            
        case SPORT_VOLLEYBALL_8_SEC:
            return COLOR_SCHEME_VOLLEYBALL;
            
        case SPORT_LACROSSE_30_SEC:
            return COLOR_SCHEME_LACROSSE;
            
        default:
            return COLOR_SCHEME_CUSTOM;
    }
}

// Main interface function that returns sport configuration based on sport type
sport_config_t get_sport_config(sport_type_t sport) {
    switch (sport) {
        case SPORT_PLAYCLOCK_NULL:
            return (sport_config_t){SPORT_PLAYCLOCK_NULL, 255, "Play Clock", "Null/Empty", PLAYCLOCK_BEHAVIOR_RESET, COLOR_SCHEME_CUSTOM};
        case SPORT_BASKETBALL_24_SEC:
            return get_basketball_config(24);
        case SPORT_BASKETBALL_30_SEC:
            return get_basketball_config(30);
        case SPORT_FOOTBALL_40_SEC:
            return get_football_config(40);
        case SPORT_FOOTBALL_25_SEC:
            return get_football_config(25);
        case SPORT_BASEBALL_15_SEC:
            return get_baseball_config(15);
        case SPORT_BASEBALL_20_SEC:
            return get_baseball_config(20);
        case SPORT_BASEBALL_14_SEC:
            return get_baseball_config(14);
        case SPORT_BASEBALL_19_SEC:
            return get_baseball_config(19);
        case SPORT_VOLLEYBALL_8_SEC:
            return (sport_config_t){SPORT_VOLLEYBALL_8_SEC, 8, "Volleyball", "8 seconds", PLAYCLOCK_BEHAVIOR_RESET, COLOR_SCHEME_VOLLEYBALL};
        case SPORT_LACROSSE_30_SEC:
            return (sport_config_t){SPORT_LACROSSE_30_SEC, 30, "Lacrosse", "30 seconds", PLAYCLOCK_BEHAVIOR_RESET, COLOR_SCHEME_LACROSSE};
        default:
            return (sport_config_t){SPORT_PLAYCLOCK_NULL, 255, "Play Clock", "Null/Empty", PLAYCLOCK_BEHAVIOR_RESET, COLOR_SCHEME_CUSTOM};
    }
}

// Generate custom sport configuration with specified count (1 or 2) and behavior
sport_config_t get_custom_config(uint8_t count, playclock_behavior_t behavior) {
    sport_config_t config = {0};
    config.play_clock_seconds = count;
    config.behavior = behavior;
    
    const char* behavior_names[] = {"Reset behavior", "Pause behavior", "Mixed behavior"};
    
    if (count == 1) {
        config.sport = SPORT_CUSTOM_ONE_RESET + behavior;
        config.name = "Custom One";
        config.variation = behavior_names[behavior];
    } else if (count == 2) {
        config.sport = SPORT_CUSTOM_TWO_RESET + behavior;
        config.name = "Custom Two";
        config.variation = behavior_names[behavior];
    }
    config.color_scheme = COLOR_SCHEME_CUSTOM;
    
    return config;
}