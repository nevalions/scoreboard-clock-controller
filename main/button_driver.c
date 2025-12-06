#include "button_driver.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <stdint.h>

static const char *TAG = "BUTTON_DRIVER";

bool button_begin(Button *button, gpio_num_t pin) {
  if (!button)
    return false;

  button->pin = pin;
  button->state = BUTTON_RELEASED;
  button->last_state = BUTTON_RELEASED;
  button->last_change_time = 0;

  // Configure GPIO with pull-up (active-low button)
  gpio_config_t io_conf = {.pin_bit_mask = (1ULL << pin),
                           .mode = GPIO_MODE_INPUT,
                           .pull_up_en = GPIO_PULLUP_ENABLE,
                           .pull_down_en = GPIO_PULLDOWN_DISABLE,
                           .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&io_conf);

  button->current_level = gpio_get_level(pin);

  ESP_LOGI(TAG, "Button initialized on GPIO %d (initial level=%d)", pin,
           button->current_level);
  return true;
}

void button_update(Button *button) {
  bool raw = gpio_get_level(button->pin); // 1=released, 0=pressed (active low)
  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

  // Change detected → start debounce window
  if (raw != button->current_level) {
    button->current_level = raw;
    button->last_change_time = now;
    return;
  }

  // Debounce complete → check if logical state must update
  if (now - button->last_change_time >= BUTTON_DEBOUNCE_MS) {
    ButtonState new_state = raw ? BUTTON_RELEASED : BUTTON_PRESSED;

    if (new_state != button->state) {

      // THIS LINE IS THE FIX
      button->last_state = button->state;

      button->state = new_state;

      ESP_LOGI("BUTTON_DRIVER", "GPIO %d: %s", button->pin,
               new_state == BUTTON_PRESSED ? "PRESSED" : "RELEASED");
    }
  }
}

bool button_is_pressed(Button *button) {
  return button && button->state == BUTTON_PRESSED;
}

bool button_get_falling_edge(Button *button) {
  if (!button)
    return false;

  bool falling = (button->last_state == BUTTON_RELEASED &&
                  button->state == BUTTON_PRESSED);

  if (falling) {
    ESP_LOGW("BUTTON_DRIVER", "EDGE: GPIO %d → FALLING (RELEASED→PRESSED)",
             button->pin);
  }

  return falling;
}

bool button_get_held(Button *button, uint32_t hold_time_ms) {
  if (!button)
    return false;

  if (button->state == BUTTON_PRESSED) {
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    return ((now - button->last_change_time) >= hold_time_ms);
  }
  return false;
}
