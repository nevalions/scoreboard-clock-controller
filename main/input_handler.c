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
// UPDATE LOOP — NOW FULLY AWARE OF UI STATE
// -----------------------------------------------------------------------------
InputAction input_handler_update(InputHandler *h, SportManager *sport_mgr,
                                 TimerManager *timer_mgr) {

  button_update(&h->control_button);
  rotary_encoder_update(&h->rotary_encoder);

  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
  InputAction action = INPUT_ACTION_NONE;

  bool pressed = button_is_pressed(&h->control_button);

  // ---------------------------------------------------------
  // Read current UI state
  // ---------------------------------------------------------
  sport_ui_state_t ui = sport_manager_get_ui_state(sport_mgr);

  // ---------------------------------------------------------
  // SHORT / LONG PRESS LOGIC ONLY VALID IN RUN MODE
  // ---------------------------------------------------------
  if (ui == SPORT_UI_STATE_RUNNING) {
    // ------------------- BUTTON DOWN -----------------------
    if (!h->button_active && pressed) {
      h->button_active = true;
      h->press_start_time = now;

      ESP_LOGD(TAG, "Button PRESS");

      // Double tap setup
      h->press_count++;
      if (h->press_count == 1) {
        h->last_press_time = now;
      } else if (h->press_count == 2) {
        if (now - h->last_press_time <= DOUBLE_TAP_MS) {

          ESP_LOGD(TAG, "Double tap → OPEN SPORT MENU");
          action = INPUT_ACTION_SPORT_SELECT; // Enter level 1
        }
        h->press_count = 0;
      }
    }

    // Timeout double tap
    if (h->press_count > 0 && now - h->last_press_time > DOUBLE_TAP_MS) {
      h->press_count = 0;
    }

    // ------------------- HOLD RESET -------------------------
    if (h->button_active && pressed &&
        (now - h->press_start_time) >= HOLD_RESET_MS) {

      ESP_LOGD(TAG, "Hold → RESET");
      timer_manager_reset(
          timer_mgr,
          sport_manager_get_current_sport(sport_mgr).play_clock_seconds);

      h->button_active = false;
      h->press_count = 0;
      return INPUT_ACTION_RESET;
    }

    // ------------------- SHORT PRESS ------------------------
    if (h->button_active && !pressed) {

      if (now - h->press_start_time < HOLD_RESET_MS) {
        ESP_LOGD(TAG, "Short press → START/STOP");
        action = INPUT_ACTION_START_STOP;
      }

      h->button_active = false;
      h->press_count = 0;
    }
  }

  // ---------------------------------------------------------
  // ROTARY SCROLL BEHAVIOR BASED ON UI STATE
  // ---------------------------------------------------------
  RotaryDirection dir = rotary_encoder_get_direction(&h->rotary_encoder);

  if (dir != ROTARY_NONE && dir != h->last_dir) {

    ESP_LOGD(TAG, "Rotary scroll dir=%d", dir);

    // =========================================================
    // LEVEL 1 → SPORT LIST
    // =========================================================
    if (ui == SPORT_UI_STATE_SELECT_SPORT) {
      ESP_LOGD(TAG, "Rotation in SPORT MENU → change selection");
      return INPUT_ACTION_SPORT_CHANGE;
    }

    // =========================================================
    // LEVEL 2 → VARIANT LIST
    //   Rotation exits back to SPORT MENU
    // =========================================================
    if (ui == SPORT_UI_STATE_SELECT_VARIANT) {
      ESP_LOGD(TAG, "Rotation in VARIANT MENU → back to SPORT MENU");
      return INPUT_ACTION_SPORT_CHANGE;
    }

    // =========================================================
    // RUN MODE (normal running clock)
    // =========================================================
    if (ui == SPORT_UI_STATE_RUNNING) {

      // Button + rotate = time adjust
      if (rotary_encoder_is_button_pressed(&h->rotary_encoder)) {

        ESP_LOGD(TAG, "Rotate + button → TIME ADJUST");
        timer_manager_adjust_time(timer_mgr, (dir == ROTARY_CW ? 1 : -1));

        return INPUT_ACTION_TIME_ADJUST;
      }

      // Rotate alone → open SPORT MENU
      ESP_LOGD(TAG, "Rotate in RUN MODE → OPEN SPORT MENU");
      return INPUT_ACTION_SPORT_CHANGE;
    }
  }

  h->last_dir = dir;

  // ---------------------------------------------------------
  // ROTARY BUTTON PRESS (short click)
  // ---------------------------------------------------------
  if (rotary_encoder_get_button_press(&h->rotary_encoder)) {

    // LEVEL 1: select sport → go to Level 2
    if (ui == SPORT_UI_STATE_SELECT_SPORT) {
      ESP_LOGD(TAG, "Rotary click → SPORT CONFIRM (enter Level 2)");
      return INPUT_ACTION_SPORT_CONFIRM;
    }

    // LEVEL 2: select default variant → running mode
    if (ui == SPORT_UI_STATE_SELECT_VARIANT) {
      ESP_LOGD(TAG, "Rotary click → CONFIRM SPORT (default variant)");
      return INPUT_ACTION_SPORT_CONFIRM;
    }

    // RUN mode rotary click = no action defined
    ESP_LOGD(TAG, "Rotary button click in RUN MODE (ignored)");
  }

  return action;
}
