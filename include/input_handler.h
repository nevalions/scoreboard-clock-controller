
#pragma once

#include "button_driver.h"
#include "driver/gpio.h"
#include "rotary_encoder.h"
#include "sport_manager.h"
#include "timer_manager.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  INPUT_ACTION_NONE,
  INPUT_ACTION_START_STOP,
  INPUT_ACTION_RESET,
  INPUT_ACTION_SPORT_CHANGE,
  INPUT_ACTION_SPORT_CONFIRM,
  INPUT_ACTION_SPORT_SELECT,
  INPUT_ACTION_TIME_ADJUST
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

void input_handler_init(InputHandler *handler, gpio_num_t control_pin,
                        gpio_num_t clk_pin, gpio_num_t dt_pin,
                        gpio_num_t sw_pin);

InputAction input_handler_update(InputHandler *handler, SportManager *sport_mgr,
                                 TimerManager *timer_mgr);
