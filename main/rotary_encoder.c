#include <stdint.h>
#include <stdbool.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "rotary_encoder.h"

// === INIT ===
bool rotary_encoder_begin(RotaryEncoder *enc, gpio_num_t clk_pin,
                          gpio_num_t dt_pin, gpio_num_t sw_pin) {
  if (!enc)
    return false;

  enc->clk_pin = clk_pin;
  enc->dt_pin = dt_pin;
  enc->sw_pin = sw_pin;

  enc->last_clk_state = 1;
  enc->last_dt_state = 1;
  enc->last_button_state = 1;

  enc->direction = ROTARY_NONE;
  enc->position = 0;

  enc->button_pressed = false;
  enc->last_button_press_state = false;
  enc->button_press_time = 0;

  enc->last_change_time = 0;

  // CLK + DT (input only, NO pullups)
  gpio_config_t io_ab = {.mode = GPIO_MODE_INPUT,
                         .intr_type = GPIO_INTR_DISABLE,
                         .pull_up_en = GPIO_PULLUP_DISABLE,
                         .pull_down_en = GPIO_PULLDOWN_DISABLE,
                         .pin_bit_mask = (1ULL << clk_pin) | (1ULL << dt_pin)};
  gpio_config(&io_ab);

  // SW (needs pull-up)
  gpio_config_t io_sw = {.mode = GPIO_MODE_INPUT,
                         .intr_type = GPIO_INTR_DISABLE,
                         .pull_up_en = GPIO_PULLUP_ENABLE,
                         .pull_down_en = GPIO_PULLDOWN_DISABLE,
                         .pin_bit_mask = (1ULL << sw_pin)};
  gpio_config(&io_sw);

  enc->last_clk_state = gpio_get_level(clk_pin);
  enc->last_dt_state = gpio_get_level(dt_pin);
  enc->last_button_state = gpio_get_level(sw_pin);

  return true;
}

// === UPDATE ===
void rotary_encoder_update(RotaryEncoder *enc) {
  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

  // --- READ PINS ---
  uint8_t clk = gpio_get_level(enc->clk_pin);
  uint8_t dt = gpio_get_level(enc->dt_pin);
  uint8_t sw = gpio_get_level(enc->sw_pin);

  // === ROTATION (simple falling-edge on CLK) ===
  if (clk != enc->last_clk_state) {
    if (clk == 0) {
      // falling edge → direction from DT
      if (dt == 1) {
        enc->direction = ROTARY_CW;
        enc->position++;
      } else {
        enc->direction = ROTARY_CCW;
        enc->position--;
      }
      enc->last_change_time = now;
    }
    enc->last_clk_state = clk;
  }

  // Clear direction after idle
  if (now - enc->last_change_time > ROTARY_IDLE_TIMEOUT_MS)
    enc->direction = ROTARY_NONE;

  // === BUTTON === (active low)
  if (sw != enc->last_button_state) {
    enc->last_button_state = sw;

    if (sw == 0) {
      enc->button_pressed = true;
      enc->button_press_time = now;
    } else {
      enc->button_pressed = false;
    }
  }
}

RotaryDirection rotary_encoder_get_direction(RotaryEncoder *enc) {
  if (!enc) return ROTARY_NONE;
  return enc->direction;
}

bool rotary_encoder_is_button_pressed(RotaryEncoder *enc) {
  if (!enc) return false;
  return enc->button_pressed;
}

bool rotary_encoder_get_button_press(RotaryEncoder *enc) {
  if (!enc) return false;
  bool pressed = enc->button_pressed && !enc->last_button_press_state;
  enc->last_button_press_state = enc->button_pressed;
  return pressed;
}
