#include "../include/button_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static const char *TAG = "BUTTON_DRIVER";

bool button_begin(Button* button, gpio_num_t pin) {
  if (!button) return false;
  
  button->pin = pin;
  button->state = BUTTON_RELEASED;
  button->last_state = BUTTON_RELEASED;
  button->last_change_time = 0;
  
  // Configure GPIO with pull-up
  gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << pin),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&io_conf);
  
  button->current_level = gpio_get_level(pin);
  ESP_LOGI(TAG, "Button initialized on GPIO %d", pin);
  return true;
}

void button_update(Button* button) {
  if (!button) return;
  
  bool current_level = gpio_get_level(button->pin);
  uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
  
  // Detect level change
  if (current_level != button->current_level) {
    button->current_level = current_level;
    button->last_change_time = current_time;
    button->last_state = button->state;
    button->state = BUTTON_DEBOUNCING;
    return;
  }
  
  // Handle debouncing
  if (button->state == BUTTON_DEBOUNCING) {
    if (current_time - button->last_change_time > 50) { // 50ms debounce
      ButtonState new_state = current_level ? BUTTON_RELEASED : BUTTON_PRESSED;
      button->last_state = button->state;
      button->state = new_state;
    }
  }
}

bool button_is_pressed(Button* button) {
  if (!button) return false;
  return button->state == BUTTON_PRESSED;
}

bool button_get_falling_edge(Button* button) {
  bool falling_edge = (button->last_state == BUTTON_RELEASED) && (button->state == BUTTON_PRESSED);
  return falling_edge;
}

bool button_get_held(Button* button, uint32_t hold_time_ms) {
  if (button->state == BUTTON_PRESSED) {
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    return (current_time - button->last_change_time) >= hold_time_ms;
  }
  return false;
}

