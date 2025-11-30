#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "input_handler.h"
#include "sport_manager.h"
#include "timer_manager.h"

#define BUTTON_DOUBLE_TAP_WINDOW_MS 500
#define BUTTON_HOLD_RESET_MS 2000
#define GPIO_DEBUG_INTERVAL_MS 1000

static const char *TAG = "INPUT_HANDLER";



void input_handler_init(InputHandler *handler, gpio_num_t control_pin, gpio_num_t clk_pin, gpio_num_t dt_pin, gpio_num_t sw_pin) {
    button_begin(&handler->control_button, control_pin);
    rotary_encoder_begin(&handler->rotary_encoder, clk_pin, dt_pin, sw_pin);
    
    handler->control_button_active = false;
    handler->control_button_press_start = 0;
    handler->last_control_button_state = BUTTON_RELEASED;
    handler->last_press_time = 0;
    handler->press_count = 0;
    handler->last_action_dir = ROTARY_NONE;
    handler->debug_last_gpio_output = 0;
}

InputAction input_handler_update(InputHandler *handler, SportManager *sport_mgr, TimerManager *timer_mgr) {
    button_update(&handler->control_button);
    rotary_encoder_update(&handler->rotary_encoder);
    
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    InputAction action = INPUT_ACTION_NONE;
    

    
    // Handle control button logic
    bool now_pressed = button_is_pressed(&handler->control_button);
    
    // Detect button press (rising edge)
    if (now_pressed && !handler->control_button_active) {
        handler->control_button_active = true;
        handler->control_button_press_start = current_time;
        ESP_LOGI(TAG, "CONTROL button pressed");
        
        // Handle double tap detection
        handler->press_count++;
        if (handler->press_count == 1) {
            handler->last_press_time = current_time;
        } else if (handler->press_count == 2) {
            if ((current_time - handler->last_press_time) <= BUTTON_DOUBLE_TAP_WINDOW_MS) {
                // Double tap detected - change sport
                sport_type_t current_sport_type = sport_manager_get_selected_type(sport_mgr);
                current_sport_type = sport_manager_get_next(current_sport_type);
                sport_manager_set_sport(sport_mgr, current_sport_type);
                action = INPUT_ACTION_SPORT_CHANGE;
                handler->press_count = 0;
            } else {
                handler->last_press_time = current_time;
                handler->press_count = 1;
            }
        }
    }
    
    // Reset double tap counter if window expires
    if (handler->press_count > 0 &&
        (current_time - handler->last_press_time) > BUTTON_DOUBLE_TAP_WINDOW_MS) {
        handler->press_count = 0;
    }
    
    // Check for hold (2 seconds) - reset timer
    if (handler->control_button_active && now_pressed &&
        (current_time - handler->control_button_press_start) >= BUTTON_HOLD_RESET_MS) {
        sport_config_t current_sport = sport_manager_get_current_sport(sport_mgr);
        timer_manager_reset(timer_mgr, current_sport.play_clock_seconds);
        action = INPUT_ACTION_RESET;
        handler->control_button_active = false;
        handler->press_count = 0;
    }
    
    // Check for release (short press) - toggle start/stop
    if (handler->control_button_active && !now_pressed) {
        if ((current_time - handler->control_button_press_start) < BUTTON_HOLD_RESET_MS) {
            timer_manager_start_stop(timer_mgr);
            action = INPUT_ACTION_START_STOP;
        }
        handler->control_button_active = false;
    }
    
    // Handle rotary encoder
    RotaryDirection dir = rotary_encoder_get_direction(&handler->rotary_encoder);
    
    if (dir != ROTARY_NONE && handler->last_action_dir == ROTARY_NONE) {
        if (rotary_encoder_is_button_pressed(&handler->rotary_encoder)) {
            // Button held while rotating → adjust time
            int adjustment = (dir == ROTARY_CW) ? 1 : -1;
            timer_manager_adjust_time(timer_mgr, adjustment);
            action = INPUT_ACTION_TIME_ADJUST;
        } else {
            // Rotation without button → navigate through sports
            sport_type_t selected_type = sport_manager_get_selected_type(sport_mgr);
            if (dir == ROTARY_CW) {
                selected_type = sport_manager_get_next(selected_type);
            } else {
                selected_type = sport_manager_get_prev(selected_type);
            }
            sport_manager_set_selected_type(sport_mgr, selected_type);
            action = INPUT_ACTION_SPORT_SELECT;
            
            // Log sport selection
            sport_config_t selected_sport = get_sport_config(selected_type);
            ESP_LOGI(TAG, "Sport SELECTED: %s %s", selected_sport.name, selected_sport.variation);
        }
        handler->last_action_dir = dir;
    }
    
    if (dir == ROTARY_NONE) {
        handler->last_action_dir = ROTARY_NONE;
    }
    
    // Handle rotary button press
    if (rotary_encoder_get_button_press(&handler->rotary_encoder)) {
        sport_config_t current_sport = sport_manager_get_current_sport(sport_mgr);
        sport_config_t selected_sport = get_sport_config(sport_manager_get_selected_type(sport_mgr));
        
        if (selected_sport.play_clock_seconds != current_sport.play_clock_seconds ||
            strcmp(selected_sport.name, current_sport.name) != 0) {
            // Different sport selected - confirm and change
            sport_manager_set_sport(sport_mgr, sport_manager_get_selected_type(sport_mgr));
            action = INPUT_ACTION_SPORT_CONFIRM;
            ESP_LOGI(TAG, "Sport CONFIRMED: %s %s (%d seconds)", 
                     selected_sport.name, selected_sport.variation, selected_sport.play_clock_seconds);
        } else {
            // Same sport selected - quick reset
            timer_manager_reset(timer_mgr, current_sport.play_clock_seconds);
            action = INPUT_ACTION_RESET;
            ESP_LOGI(TAG, "Quick reset to %d seconds", current_sport.play_clock_seconds);
        }
    }
    
    return action;
}

void input_handler_debug_gpio(const InputHandler *handler) {
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if ((current_time - handler->debug_last_gpio_output) >= GPIO_DEBUG_INTERVAL_MS) {
        int sw_raw = gpio_get_level(ROTARY_SW_PIN);
        ESP_LOGI(TAG, "GPIO level - Control: %d | Button state - Control: %d | SW raw: %d",
                 gpio_get_level(CONTROL_BUTTON_PIN),
                 button_is_pressed((Button*)&handler->control_button), sw_raw);
        ESP_LOGI("SWTEST", "SW raw level: %d", sw_raw);
    }
}