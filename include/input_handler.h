#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "button_driver.h"
#include "rotary_encoder.h"
#include "sport_manager.h"
#include "timer_manager.h"

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
    bool control_button_active;
    uint32_t control_button_press_start;
    ButtonState last_control_button_state;
    uint32_t last_press_time;
    uint32_t press_count;
    RotaryDirection last_action_dir;
    uint32_t debug_last_gpio_output;
} InputHandler;

void input_handler_init(InputHandler *handler, gpio_num_t control_pin, gpio_num_t clk_pin, gpio_num_t dt_pin, gpio_num_t sw_pin);
InputAction input_handler_update(InputHandler *handler, SportManager *sport_mgr, TimerManager *timer_mgr);
void input_handler_debug_gpio(const InputHandler *handler);