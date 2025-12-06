#include "input_handler.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define DOUBLE_TAP_MS 500
#define HOLD_RESET_MS 2000

static const char *TAG = "INPUT_HANDLER";

// Rotary debounce
static uint32_t s_last_rotary_action_ms = 0;

// -----------------------------------------------------------------------------
// INIT
// -----------------------------------------------------------------------------
void input_handler_init(InputHandler *h, gpio_num_t control_pin,
                        gpio_num_t clk_pin, gpio_num_t dt_pin,
                        gpio_num_t sw_pin, gpio_num_t preset1_pin,
                        gpio_num_t preset2_pin, gpio_num_t preset3_pin,
                        gpio_num_t preset4_pin, gpio_num_t start_pin,
                        gpio_num_t reset_pin) {
  ESP_LOGI(TAG, "Initializing InputHandler...");

  // internal button
  button_begin(&h->control_button, control_pin);

  // rotary
  rotary_encoder_begin(&h->rotary_encoder, clk_pin, dt_pin, sw_pin);

  // preset buttons
  button_begin(&h->preset_buttons[0], preset1_pin);
  button_begin(&h->preset_buttons[1], preset2_pin);
  button_begin(&h->preset_buttons[2], preset3_pin);
  button_begin(&h->preset_buttons[3], preset4_pin);

  // start/pause & reset
  button_begin(&h->start_button, start_pin);
  button_begin(&h->reset_button, reset_pin);

  h->button_active = false;
  h->press_start_time = 0;
  h->last_press_time = 0;
  h->press_count = 0;
  h->last_dir = ROTARY_NONE;

  ESP_LOGI(TAG, "InputHandler initialized");
}

// -----------------------------------------------------------------------------
// UPDATE
// -----------------------------------------------------------------------------
InputAction input_handler_update(InputHandler *h, SportManager *sport_mgr,
                                 TimerManager *timer_mgr) {
  // Update ALL hardware buttons
  button_update(&h->control_button);
  for (int i = 0; i < 4; i++)
    button_update(&h->preset_buttons[i]);
  button_update(&h->start_button);
  button_update(&h->reset_button);

  rotary_encoder_update(&h->rotary_encoder);

  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

  InputAction action = INPUT_ACTION_NONE;
  sport_ui_state_t ui = sport_manager_get_ui_state(sport_mgr);

  bool internal_pressed = button_is_pressed(&h->control_button);

  ESP_LOGD(TAG, "UI state in input_handler_update = %d", ui);

  // -------------------------------------------------------------------------
  // EXTERNAL BUTTONS — always generate actions
  // (main.c decides whether to act based on ui_state)
  // -------------------------------------------------------------------------
  if (button_get_falling_edge(&h->start_button)) {
    ESP_LOGW(TAG, "START/Pause button pressed!");
    return INPUT_ACTION_START_STOP;
  }

  if (button_get_falling_edge(&h->reset_button)) {
    ESP_LOGW(TAG, "RESET button pressed!");
    return INPUT_ACTION_RESET;
  }

  for (int i = 0; i < 4; i++) {
    if (button_get_falling_edge(&h->preset_buttons[i])) {
      ESP_LOGW(TAG, "Preset button %d clicked!", i + 1);
      return INPUT_ACTION_PRESET_1 + i;
    }
  }

  // -------------------------------------------------------------------------
  // INTERNAL BUTTON: short press, long press, double-tap
  // (only meaningful in RUN mode, so we keep the guard here)
  // -------------------------------------------------------------------------
  if (ui == SPORT_UI_STATE_RUNNING) {
    if (!h->button_active && internal_pressed) {
      h->button_active = true;
      h->press_start_time = now;

      h->press_count++;
      h->last_press_time = now;

      ESP_LOGI(TAG, "Internal button pressed");
    }

    if (internal_pressed && (now - h->press_start_time >= HOLD_RESET_MS)) {
      ESP_LOGW(TAG, "Internal HOLD → RESET");
      h->button_active = false;
      h->press_count = 0;
      return INPUT_ACTION_RESET;
    }

    if (h->button_active && !internal_pressed) {
      // short press
      if (now - h->press_start_time < HOLD_RESET_MS) {
        ESP_LOGW(TAG, "Internal short press → start/stop");
        return INPUT_ACTION_START_STOP;
      }
      h->button_active = false;
      h->press_count = 0;
    }

    // double tap
    if (h->press_count == 2 && now - h->last_press_time < DOUBLE_TAP_MS) {
      h->press_count = 0;
      ESP_LOGW(TAG, "Internal double tap → open sport menu");
      return INPUT_ACTION_SPORT_SELECT;
    }

    if (h->press_count == 1 && now - h->last_press_time > DOUBLE_TAP_MS) {
      h->press_count = 0;
    }
  }

  // -------------------------------------------------------------------------
  // ROTARY SCROLL
  // -------------------------------------------------------------------------
  RotaryDirection dir = rotary_encoder_get_direction(&h->rotary_encoder);

  if (dir != ROTARY_NONE) {
    if (now - s_last_rotary_action_ms >= 120) {

      s_last_rotary_action_ms = now;

      ESP_LOGI(TAG, "Rotary scroll: %s", dir == ROTARY_CW ? "CW" : "CCW");

      InputAction scroll = (dir == ROTARY_CW ? INPUT_ACTION_SPORT_NEXT
                                             : INPUT_ACTION_SPORT_PREV);

      if (ui == SPORT_UI_STATE_SELECT_SPORT)
        return scroll;
      if (ui == SPORT_UI_STATE_SELECT_VARIANT)
        return scroll;
      if (ui == SPORT_UI_STATE_RUNNING)
        return INPUT_ACTION_SPORT_SELECT;
    }
  }

  // -------------------------------------------------------------------------
  // ROTARY CLICK
  // -------------------------------------------------------------------------
  if (rotary_encoder_get_button_press(&h->rotary_encoder)) {
    ESP_LOGI(TAG, "Rotary button click detected");

    if (ui == SPORT_UI_STATE_SELECT_SPORT)
      return INPUT_ACTION_SPORT_CONFIRM;
    if (ui == SPORT_UI_STATE_SELECT_VARIANT)
      return INPUT_ACTION_SPORT_CONFIRM;
  }

  return action;
}
