#pragma once

#include "driver/gpio.h"
#include <stdbool.h>
#include <stdint.h>

// Button timing constants
#define BUTTON_DEBOUNCE_MS 50 // Debounce delay in milliseconds

// Button states
typedef enum {
  BUTTON_RELEASED = 0,
  BUTTON_PRESSED = 1,
  BUTTON_DEBOUNCING = 2
} ButtonState;

// Button structure
typedef struct {
  gpio_num_t pin;

  // Debounced logical state (PRESSED/RELEASED)
  ButtonState state;
  ButtonState last_state;

  // Raw stable level (after debounce timer)
  bool current_level;

  // Timestamp when level last changed
  uint32_t last_change_time;

  // NEW: ensures falling edge fires only once per press
  bool falling_edge_consumed;

} Button;

// API
bool button_begin(Button *button, gpio_num_t pin);
void button_update(Button *button);

bool button_is_pressed(Button *button);

// One-shot edge detection (fires once per press)
bool button_get_falling_edge(Button *button);

// Standard held-time check
bool button_get_held(Button *button, uint32_t hold_time_ms);
