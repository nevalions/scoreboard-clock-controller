#include "button_driver.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <stdint.h>

bool button_begin(Button *btn, gpio_num_t pin) {
  btn->pin = pin;

  gpio_config_t io_conf;
  memset(&io_conf, 0, sizeof(io_conf)); // IMPORTANT FIX

  io_conf.pin_bit_mask = (1ULL << pin);
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

  // Enable internal pull-up only if the pin supports it
  if (pin < 34) {
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  }

  ESP_ERROR_CHECK(gpio_config(&io_conf));

  btn->current_level = gpio_get_level(pin);
  btn->state = BUTTON_RELEASED;
  btn->last_state = BUTTON_RELEASED;
  btn->last_change_time = 0;
  btn->falling_edge_consumed = false;

  ESP_LOGI("BUTTON_DRIVER", "Button initialized on GPIO %d (initial=%d, PU=%d)",
           pin, btn->current_level, io_conf.pull_up_en);

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

  if (falling && !button->falling_edge_consumed) {
    button->falling_edge_consumed = true;
    ESP_LOGW("BUTTON_DRIVER", "EDGE: GPIO %d → FALLING (one-shot)",
             button->pin);
    return true;
  }

  // Reset after release
  if (button->state == BUTTON_RELEASED) {
    button->falling_edge_consumed = false;
  }

  return false;
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
