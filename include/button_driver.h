#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"

// Button pins
#define CONTROL_BUTTON_PIN   GPIO_NUM_0  // Single button for start/stop/reset

// Button timing constants
#define BUTTON_DEBOUNCE_MS   50  // Debounce delay in milliseconds

// Button states
typedef enum {
    BUTTON_RELEASED = 0,
    BUTTON_PRESSED = 1,
    BUTTON_DEBOUNCING = 2
} ButtonState;

// Button structure
typedef struct {
    gpio_num_t pin;
    ButtonState state;
    ButtonState last_state;
    uint32_t last_change_time;
    bool current_level;
} Button;

// Function declarations
bool button_begin(Button* button, gpio_num_t pin);
bool button_is_pressed(Button* button);
void button_update(Button* button);
bool button_get_falling_edge(Button* button);
bool button_get_held(Button* button, uint32_t hold_time_ms);