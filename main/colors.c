#include "../include/colors.h"

// Seconds remaining below which football switches to the urgent color
#define URGENT_COUNTDOWN_THRESHOLD_SEC 5

color_t get_sport_color(color_scheme_t scheme, uint8_t seconds) {
    switch (scheme) {
        case COLOR_SCHEME_FOOTBALL:
            // Football: urgent color inside the final seconds, red at zero
            if (seconds == 0xFF || seconds == 0) {
                return COLOR_RED;
            } else if (seconds < URGENT_COUNTDOWN_THRESHOLD_SEC) {
                return COLOR_DEEP_ORANGE;
            } else {
                return COLOR_ORANGE;
            }
            
        case COLOR_SCHEME_BASKETBALL:
            // Basketball: always red
            return COLOR_RED;
            
        case COLOR_SCHEME_BASEBALL:
        case COLOR_SCHEME_VOLLEYBALL:
        case COLOR_SCHEME_LACROSSE:
        case COLOR_SCHEME_CUSTOM:
        default:
            // Other sports: always orange
            return COLOR_ORANGE;
    }
}