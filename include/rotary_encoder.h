#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"

// KY-040 Rotary Encoder pins
#define ROTARY_CLK_PIN      GPIO_NUM_34  // Clock pin (A)
#define ROTARY_DT_PIN       GPIO_NUM_35  // Data pin (B)
#define ROTARY_SW_PIN       GPIO_NUM_32  // Switch pin (button)

// Rotary encoder directions
typedef enum {
    ROTARY_NONE = 0,
    ROTARY_CW = 1,        // Clockwise
    ROTARY_CCW = -1       // Counter-clockwise
} RotaryDirection;

// Rotary encoder structure
typedef struct {
    gpio_num_t clk_pin;
    gpio_num_t dt_pin;
    gpio_num_t sw_pin;
    uint8_t last_clk_state;
    uint8_t last_dt_state;
    int32_t position;
    RotaryDirection direction;
    uint32_t last_change_time;
    bool button_pressed;
    bool last_button_state;
    uint32_t button_press_time;
    bool last_button_press_state;
} RotaryEncoder;

// Function declarations
bool rotary_encoder_begin(RotaryEncoder* encoder, gpio_num_t clk_pin, gpio_num_t dt_pin, gpio_num_t sw_pin);
void rotary_encoder_update(RotaryEncoder* encoder);
RotaryDirection rotary_encoder_get_direction(RotaryEncoder* encoder);
int32_t rotary_encoder_get_position(RotaryEncoder* encoder);
void rotary_encoder_reset_position(RotaryEncoder* encoder);
bool rotary_encoder_is_button_pressed(RotaryEncoder* encoder);
bool rotary_encoder_get_button_press(RotaryEncoder* encoder);
bool rotary_encoder_get_button_held(RotaryEncoder* encoder, uint32_t hold_time_ms);