#ifndef COLORS_H
#define COLORS_H

#include <stdint.h>

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} color_t;

// Standard color definitions
#define COLOR_RED ((color_t){255, 0, 0})
#define COLOR_ORANGE ((color_t){255, 90, 0})
#define COLOR_DEEP_ORANGE ((color_t){255, 40, 0})
#define COLOR_GREEN ((color_t){0, 255, 0})
#define COLOR_BLUE ((color_t){0, 0, 255})
#define COLOR_WHITE ((color_t){255, 255, 255})
#define COLOR_BLACK ((color_t){0, 0, 0})

// Sport-specific color schemes
typedef enum {
  COLOR_SCHEME_FOOTBALL,   // Changes after 5 seconds
  COLOR_SCHEME_BASKETBALL, // Always red
  COLOR_SCHEME_BASEBALL,   // Always orange
  COLOR_SCHEME_VOLLEYBALL, // Always orange
  COLOR_SCHEME_LACROSSE,   // Always orange
  COLOR_SCHEME_CUSTOM      // Default orange behavior
} color_scheme_t;

// Function to get color based on sport and time value
color_t get_sport_color(color_scheme_t scheme, uint8_t seconds);

#endif
