#include "rotary_encoder.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// INIT
// ============================================================================
bool rotary_encoder_begin(RotaryEncoder *enc, gpio_num_t clk_pin,
                          gpio_num_t dt_pin, gpio_num_t sw_pin) {
  if (!enc)
    return false;

  enc->clk_pin = clk_pin;
  enc->dt_pin = dt_pin;
  enc->sw_pin = sw_pin;

  enc->direction = ROTARY_NONE;
  enc->position = 0;

  enc->button_pressed = false;
  enc->last_button_press_state = false;
  enc->button_press_time = 0;

  enc->last_change_time = 0;

  // ------------------------------------------------------------------------
  // Configure CLK + DT inputs (KY-040 already has hardware pull-ups)
  // ------------------------------------------------------------------------
  gpio_config_t io_ab = {.mode = GPIO_MODE_INPUT,
                         .intr_type = GPIO_INTR_DISABLE,
                         .pull_up_en = GPIO_PULLUP_DISABLE,
                         .pull_down_en = GPIO_PULLDOWN_DISABLE,
                         .pin_bit_mask = (1ULL << clk_pin) | (1ULL << dt_pin)};
  gpio_config(&io_ab);

  // ------------------------------------------------------------------------
  // Configure SWITCH input (needs pull-up)
  // ------------------------------------------------------------------------
  gpio_config_t io_sw = {.mode = GPIO_MODE_INPUT,
                         .intr_type = GPIO_INTR_DISABLE,
                         .pull_up_en = GPIO_PULLUP_ENABLE,
                         .pull_down_en = GPIO_PULLDOWN_DISABLE,
                         .pin_bit_mask = (1ULL << sw_pin)};
  gpio_config(&io_sw);

  // Initial states
  enc->last_clk_state = gpio_get_level(clk_pin);
  enc->last_dt_state = gpio_get_level(dt_pin);
  enc->last_button_state = gpio_get_level(sw_pin);

  // NEW: initialize quadrature combined state
  enc->last_encoded_state = (enc->last_clk_state << 1) | enc->last_dt_state;

  return true;
}

// ============================================================================
// UPDATE (Quadrature-decoder version — stable & accurate)
// ============================================================================
void rotary_encoder_update(RotaryEncoder *enc) {
  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

  // Read raw pin states
  uint8_t clk = gpio_get_level(enc->clk_pin);
  uint8_t dt = gpio_get_level(enc->dt_pin);
  uint8_t sw = gpio_get_level(enc->sw_pin);

  // ------------------------------------------------------------------------
  // QUADRATURE DECODING
  // ------------------------------------------------------------------------

  // Transition table for 2-bit quadrature decoder
  static const int8_t transition_table[16] = {0,  -1, 1, 0, 1, 0, 0,  -1,
                                              -1, 0,  0, 1, 0, 1, -1, 0};

  uint8_t new_state = (clk << 1) | dt;
  uint8_t old_state = enc->last_encoded_state;
  uint8_t index = (old_state << 2) | new_state;

  int8_t movement = transition_table[index];

  // // DEBUG QUADRATURE LOG
  // ESP_LOGI("ENC", "Quad: old=%u new=%u index=%u movement=%d clk=%d dt=%d",
  //          old_state, new_state, index, movement, clk, dt);

  enc->last_encoded_state = new_state;

  if (movement != 0) {
    // Debounce rotation (ignore jitter faster than ~2ms)
    if (now - enc->last_change_time > 2) {
      enc->position += movement;

      if (movement > 0)
        enc->direction = ROTARY_CW;
      else
        enc->direction = ROTARY_CCW;

      enc->last_change_time = now;

      // // DEBUG MOVEMENT LOG
      // ESP_LOGI("ENC", "MOVE dir=%s pos=%d", (movement > 0 ? "CW" : "CCW"),
      //          enc->position);
    }
  }

  // After 40ms idle, mark no movement
  if (now - enc->last_change_time > 40) {
    enc->direction = ROTARY_NONE;
  }

  // ------------------------------------------------------------------------
  // BUTTON HANDLING (active low)
  // ------------------------------------------------------------------------
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

// ============================================================================
// API FUNCTIONS
// ============================================================================
RotaryDirection rotary_encoder_get_direction(RotaryEncoder *enc) {
  if (!enc)
    return ROTARY_NONE;
  return enc->direction;
}

bool rotary_encoder_is_button_pressed(RotaryEncoder *enc) {
  if (!enc)
    return false;
  return enc->button_pressed;
}

bool rotary_encoder_get_button_press(RotaryEncoder *enc) {
  if (!enc)
    return false;

  bool is_new_press = enc->button_pressed && !enc->last_button_press_state;

  enc->last_button_press_state = enc->button_pressed;
  return is_new_press;
}
