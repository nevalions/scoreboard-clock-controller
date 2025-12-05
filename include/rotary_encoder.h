#pragma once

#include "driver/gpio.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum { ROTARY_NONE = 0, ROTARY_CW, ROTARY_CCW } RotaryDirection;

typedef struct {
  gpio_num_t clk_pin;
  gpio_num_t dt_pin;
  gpio_num_t sw_pin;

  // Raw pin states
  uint8_t last_clk_state;
  uint8_t last_dt_state;
  uint8_t last_button_state;

  // Quadrature combined state (NEW)
  uint8_t last_encoded_state;

  // Movement info
  RotaryDirection direction;
  int32_t position;

  // Button logic
  bool button_pressed;
  bool last_button_press_state;
  uint32_t button_press_time;

  // Debounce timing
  uint32_t last_change_time;

} RotaryEncoder;

// API
bool rotary_encoder_begin(RotaryEncoder *enc, gpio_num_t clk_pin,
                          gpio_num_t dt_pin, gpio_num_t sw_pin);

void rotary_encoder_update(RotaryEncoder *enc);

RotaryDirection rotary_encoder_get_direction(RotaryEncoder *enc);

bool rotary_encoder_is_button_pressed(RotaryEncoder *enc);

bool rotary_encoder_get_button_press(RotaryEncoder *enc);
