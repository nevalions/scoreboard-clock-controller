#include "../include/rotary_encoder.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static const char *TAG = "ROTARY_ENCODER";

bool rotary_encoder_begin(RotaryEncoder* encoder, gpio_num_t clk_pin, gpio_num_t dt_pin, gpio_num_t sw_pin) {
    if (!encoder) return false;
    
    encoder->clk_pin = clk_pin;
    encoder->dt_pin = dt_pin;
    encoder->sw_pin = sw_pin;
    encoder->last_clk_state = 1;
    encoder->last_dt_state = 1;
    encoder->position = 0;
    encoder->direction = ROTARY_NONE;
    encoder->last_change_time = 0;
    encoder->button_pressed = false;
    encoder->last_button_state = true;
    encoder->button_press_time = 0;
    encoder->last_button_press_state = false;

    // Configure CLK pin (GPIO34 is input-only, no pull-up)
    gpio_config_t clk_conf = {
        .pin_bit_mask = (1ULL << clk_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,  // GPIO34 has no internal pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&clk_conf);

    // Configure DT pin (GPIO35 is input-only, no pull-up)
    gpio_config_t dt_conf = {
        .pin_bit_mask = (1ULL << dt_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,  // GPIO35 has no internal pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&dt_conf);

    // Configure SW pin (button)
    gpio_config_t sw_conf = {
        .pin_bit_mask = (1ULL << sw_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&sw_conf);

    // Read initial states
    encoder->last_clk_state = gpio_get_level(clk_pin);
    encoder->last_dt_state = gpio_get_level(dt_pin);
    encoder->last_button_state = gpio_get_level(sw_pin);

    ESP_LOGI(TAG, "Rotary encoder initialized - CLK: GPIO%d, DT: GPIO%d, SW: GPIO%d", 
             clk_pin, dt_pin, sw_pin);
    return true;
}

void rotary_encoder_update(RotaryEncoder* encoder) {
    if (!encoder) return;
    
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Read current states
    uint8_t current_clk_state = gpio_get_level(encoder->clk_pin);
    uint8_t current_dt_state = gpio_get_level(encoder->dt_pin);
    bool current_button_state = gpio_get_level(encoder->sw_pin);

    // Detect rotary encoder movement
    if (current_clk_state != encoder->last_clk_state) {
        if (current_clk_state == 0) { // Falling edge on CLK
            if (current_dt_state == 0) {
                encoder->direction = ROTARY_CW;
                encoder->position++;
            } else {
                encoder->direction = ROTARY_CCW;
                encoder->position--;
            }
            encoder->last_change_time = current_time;
            
            ESP_LOGD(TAG, "Rotary encoder: %s, position: %d", 
                     encoder->direction == ROTARY_CW ? "CW" : "CCW", 
                     encoder->position);
        }
        encoder->last_clk_state = current_clk_state;
    }

    // Update button state
    if (current_button_state != encoder->last_button_state) {
        encoder->last_button_state = current_button_state;
        if (current_button_state == 0) { // Button pressed (active low)
            encoder->button_pressed = true;
            encoder->button_press_time = current_time;
            ESP_LOGD(TAG, "Rotary encoder button pressed");
        } else { // Button released
            encoder->button_pressed = false;
            ESP_LOGD(TAG, "Rotary encoder button released");
        }
    }

    // Reset direction after a period of no movement
    if (current_time - encoder->last_change_time > 100) {
        encoder->direction = ROTARY_NONE;
    }
}

RotaryDirection rotary_encoder_get_direction(RotaryEncoder* encoder) {
    if (!encoder) return ROTARY_NONE;
    return encoder->direction;
}

int32_t rotary_encoder_get_position(RotaryEncoder* encoder) {
    return encoder->position;
}

void rotary_encoder_reset_position(RotaryEncoder* encoder) {
    encoder->position = 0;
    encoder->direction = ROTARY_NONE;
    ESP_LOGD(TAG, "Rotary encoder position reset");
}

bool rotary_encoder_is_button_pressed(RotaryEncoder* encoder) {
    return encoder->button_pressed;
}

bool rotary_encoder_get_button_press(RotaryEncoder* encoder) {
    if (!encoder) return false;
    
    bool current_pressed = encoder->button_pressed;
    bool press_detected = current_pressed && !encoder->last_button_press_state;
    encoder->last_button_press_state = current_pressed;
    return press_detected;
}

bool rotary_encoder_get_button_held(RotaryEncoder* encoder, uint32_t hold_time_ms) {
    if (encoder->button_pressed) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        return (current_time - encoder->button_press_time) >= hold_time_ms;
    }
    return false;
}