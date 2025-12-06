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

  INPUT_ACTION_START_STOP = 1,
  INPUT_ACTION_RESET = 2,
  INPUT_ACTION_TIME_ADJUST = 3,

  INPUT_ACTION_SPORT_SELECT = 4,
  INPUT_ACTION_SPORT_CONFIRM = 5,

  INPUT_ACTION_SPORT_NEXT = 6,
  INPUT_ACTION_SPORT_PREV = 7,

  // Assign preset IDs explicitly
  INPUT_ACTION_PRESET_1 = 8,
  INPUT_ACTION_PRESET_2 = 9,
  INPUT_ACTION_PRESET_3 = 10,
  INPUT_ACTION_PRESET_4 = 11
} InputAction;

typedef struct {
  // Existing inputs
  Button control_button; // on-board/internal control button
  RotaryEncoder rotary_encoder;

  // New external hardware buttons
  Button preset_buttons[4]; // up to 4 playclock presets for current sport
  Button start_button;      // dedicated START/PAUSE button
  Button reset_button;      // dedicated RESET button

  // State for internal button gesture detection
  bool button_active;
  uint32_t press_start_time;
  uint32_t last_press_time;
  uint8_t press_count;

  RotaryDirection last_dir;
} InputHandler;

void input_handler_init(InputHandler *h, gpio_num_t control_pin,
                        gpio_num_t clk_pin, gpio_num_t dt_pin,
                        gpio_num_t sw_pin, gpio_num_t preset1_pin,
                        gpio_num_t preset2_pin, gpio_num_t preset3_pin,
                        gpio_num_t preset4_pin, gpio_num_t start_pin,
                        gpio_num_t reset_pin);

InputAction input_handler_update(InputHandler *h, SportManager *sport_mgr,
                                 TimerManager *timer_mgr);
