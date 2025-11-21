#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"

// Button pins
#define START_BUTTON_PIN    GPIO_NUM_0
#define STOP_BUTTON_PIN     GPIO_NUM_2
#define RESET_BUTTON_PIN    GPIO_NUM_15

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
    uint32_t last_change_time;
    bool current_level;
} Button;

// Function declarations
bool button_begin(Button* button, gpio_num_t pin);
bool button_is_pressed(Button* button);
void button_update(Button* button);
bool button_get_falling_edge(Button* button);