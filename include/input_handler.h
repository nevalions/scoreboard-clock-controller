#pragma once

#include "button_driver.h"
#include "driver/gpio.h"
#include "rotary_encoder.h"
#include "sport_manager.h"
#include "timer_manager.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  INPUT_ACTION_NONE = 0,

  INPUT_ACTION_START_STOP,
  INPUT_ACTION_RESET,
  INPUT_ACTION_TIME_ADJUST,

  INPUT_ACTION_SPORT_SELECT,  // enter / exit menus
  INPUT_ACTION_SPORT_CONFIRM, // confirm sport / variant

  INPUT_ACTION_SPORT_NEXT, // rotary CW in sport/variant menu
  INPUT_ACTION_SPORT_PREV  // rotary CCW in sport/variant menu
} InputAction;

typedef struct {
  Button control_button;
  RotaryEncoder rotary_encoder;

  bool button_active;
  uint32_t press_start_time;
  uint32_t last_press_time;
  uint8_t press_count;

  RotaryDirection last_dir;
} InputHandler;

void input_handler_init(InputHandler *h, gpio_num_t control_pin,
                        gpio_num_t clk_pin, gpio_num_t dt_pin,
                        gpio_num_t sw_pin);

InputAction input_handler_update(InputHandler *h, SportManager *sport_mgr,
                                 TimerManager *timer_mgr);
