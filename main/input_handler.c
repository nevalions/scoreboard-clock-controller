#include "input_handler.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define DOUBLE_TAP_MS 500
#define HOLD_RESET_MS 2000

static const char *TAG = "INPUT_HANDLER";

// -----------------------------------------------------------------------------
// INIT
// -----------------------------------------------------------------------------
void input_handler_init(InputHandler *h, gpio_num_t control_pin,
                        gpio_num_t clk_pin, gpio_num_t dt_pin,
                        gpio_num_t sw_pin) {
  button_begin(&h->control_button, control_pin);
  rotary_encoder_begin(&h->rotary_encoder, clk_pin, dt_pin, sw_pin);

  h->button_active = false;
  h->press_start_time = 0;
  h->last_press_time = 0;
  h->press_count = 0;
  h->last_dir = ROTARY_NONE;
}

// -----------------------------------------------------------------------------
// UPDATE LOOP
// -----------------------------------------------------------------------------
InputAction input_handler_update(InputHandler *h, SportManager *sport_mgr,
                                 TimerManager *timer_mgr) {

  button_update(&h->control_button);
  rotary_encoder_update(&h->rotary_encoder);

  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
  InputAction action = INPUT_ACTION_NONE;

  bool pressed = button_is_pressed(&h->control_button);

  // ---------------------------------------------------------
  // BUTTON — RISING EDGE
  // ---------------------------------------------------------
  if (!h->button_active && pressed) {
    h->button_active = true;
    h->press_start_time = now;

    ESP_LOGD(TAG, "Control button PRESS");

    // Double-tap logic
    h->press_count++;
    if (h->press_count == 1) {
      h->last_press_time = now;
    } else if (h->press_count == 2) {
      if (now - h->last_press_time <= DOUBLE_TAP_MS) {

        ESP_LOGD(TAG, "Double-tap → SPORT CHANGE");

        sport_type_t t = sport_manager_get_selected_type(sport_mgr);
        t = sport_manager_get_next(t);
        sport_manager_set_sport(sport_mgr, t);

        action = INPUT_ACTION_SPORT_CHANGE;
      }
      h->press_count = 0;
    }
  }

  // Double-tap timeout
  if (h->press_count > 0 && now - h->last_press_time > DOUBLE_TAP_MS) {
    h->press_count = 0;
  }

  // ---------------------------------------------------------
  // BUTTON — HOLD → RESET (2 seconds)
  // ---------------------------------------------------------
  if (h->button_active && pressed &&
      (now - h->press_start_time) >= HOLD_RESET_MS) {

    ESP_LOGD(TAG, "Button HOLD → RESET");

    sport_config_t s = sport_manager_get_current_sport(sport_mgr);
    timer_manager_reset(timer_mgr, s.play_clock_seconds);

    action = INPUT_ACTION_RESET;

    h->button_active = false;
    h->press_count = 0;
    return action;
  }

  // ---------------------------------------------------------
  // BUTTON — RELEASE EDGE → START/STOP
  // ---------------------------------------------------------
  if (h->button_active && !pressed) {

    if (now - h->press_start_time < HOLD_RESET_MS) {
      ESP_LOGD(TAG, "Button short press → START/STOP");
      action = INPUT_ACTION_START_STOP;
    }

    h->button_active = false;
    h->press_count = 0;
  }

  // ---------------------------------------------------------
  // ROTARY SCROLL
  // ---------------------------------------------------------
  RotaryDirection dir = rotary_encoder_get_direction(&h->rotary_encoder);

  if (dir != ROTARY_NONE) {
    if (dir != h->last_dir) {

      ESP_LOGD(TAG, "Rotary scroll dir=%d", dir);

      if (rotary_encoder_is_button_pressed(&h->rotary_encoder)) {
        // TIME ADJUST (button held)
        ESP_LOGD(TAG, "Rotary + Button → TIME ADJUST");

        timer_manager_adjust_time(timer_mgr, (dir == ROTARY_CW ? 1 : -1));
        action = INPUT_ACTION_TIME_ADJUST;

      } else {
        // SPORT SELECT ONLY
        ESP_LOGD(TAG, "Rotary scroll → SPORT_SELECT");

        sport_type_t t = sport_manager_get_selected_type(sport_mgr);
        t = (dir == ROTARY_CW) ? sport_manager_get_next(t)
                               : sport_manager_get_prev(t);

        sport_manager_set_selected_type(sport_mgr, t);
        action = INPUT_ACTION_SPORT_SELECT;
      }

      h->last_dir = dir;
    }
  } else {
    h->last_dir = ROTARY_NONE;
  }

  // ---------------------------------------------------------
  // ROTARY BUTTON PRESS → Confirm sport OR quick reset
  // ---------------------------------------------------------
  if (rotary_encoder_get_button_press(&h->rotary_encoder)) {

    ESP_LOGD(TAG, "Rotary BUTTON PRESS");

    sport_config_t cur = sport_manager_get_current_sport(sport_mgr);
    sport_type_t sel_type = sport_manager_get_selected_type(sport_mgr);
    sport_config_t selected = get_sport_config(sel_type);

    bool same_sport = (cur.play_clock_seconds == selected.play_clock_seconds) &&
                      (strcmp(cur.name, selected.name) == 0);

    if (!same_sport) {
      ESP_LOGD(TAG, "Sport confirmed: APPLY NEW");
      sport_manager_set_sport(sport_mgr, sel_type);
      action = INPUT_ACTION_SPORT_CONFIRM;

    } else {
      ESP_LOGD(TAG, "Sport confirmed: SAME → RESET");
      timer_manager_reset(timer_mgr, cur.play_clock_seconds);
      action = INPUT_ACTION_RESET;
    }
  }

  return action;
}
