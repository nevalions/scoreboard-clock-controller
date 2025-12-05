#include "input_handler.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define DOUBLE_TAP_MS 500
#define HOLD_RESET_MS 2000

static const char *TAG = "INPUT_HANDLER";

// UI-level debounce for rotary actions (one logical step per ~120 ms)
static uint32_t s_last_rotary_action_ms = 0;

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
// UPDATE LOOP — aware of sport UI state
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
  // SHORT / LONG PRESS LOGIC (only valid in RUN mode)
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

  if (dir != ROTARY_NONE) {

    // === UI-LEVEL DEBOUNCE: one logical action / 120ms ===
    if (now - s_last_rotary_action_ms >= 120) {
      s_last_rotary_action_ms = now;

      ESP_LOGD(TAG, "Rotary scroll dir=%d (%s)", dir,
               (dir == ROTARY_CW ? "CW" : "CCW"));

      // Determine scroll action for menus
      InputAction scroll_action = (dir == ROTARY_CW) ? INPUT_ACTION_SPORT_NEXT
                                                     : INPUT_ACTION_SPORT_PREV;

      // =====================================================
      // LEVEL 1 → SPORT LIST
      // =====================================================
      if (ui == SPORT_UI_STATE_SELECT_SPORT) {
        ESP_LOGD(TAG, "Rotation in SPORT MENU → %s",
                 (dir == ROTARY_CW ? "NEXT" : "PREV"));
        return scroll_action;
      }

      // =====================================================
      // LEVEL 2 → VARIANT LIST
      //  For now: any rotation leaves variant menu
      // =====================================================
      if (ui == SPORT_UI_STATE_SELECT_VARIANT) {
        ESP_LOGD(TAG, "Rotation in VARIANT MENU → back to SPORT MENU");
        return scroll_action;
      }

      // =====================================================
      // RUN MODE (normal running clock)
      // =====================================================
      if (ui == SPORT_UI_STATE_RUNNING) {

        // Button + rotate = time adjust (we DO use direction here)
        if (rotary_encoder_is_button_pressed(&h->rotary_encoder)) {

          ESP_LOGD(TAG, "Rotate + button → TIME ADJUST (%s)",
                   (dir == ROTARY_CW ? "+1" : "-1"));
          timer_manager_adjust_time(timer_mgr, (dir == ROTARY_CW ? 1 : -1));

          return INPUT_ACTION_TIME_ADJUST;
        }

        // Rotate alone → open SPORT MENU
        ESP_LOGD(TAG, "Rotate in RUN MODE → OPEN SPORT MENU");
        return INPUT_ACTION_SPORT_SELECT;
      }
    } else {
      // ESP_LOGD(TAG, "Rotary movement ignored (debounce)");
    }
  }

  h->last_dir = dir;

  // ---------------------------------------------------------
  // ROTARY BUTTON PRESS (short click)
  // ---------------------------------------------------------
  if (rotary_encoder_get_button_press(&h->rotary_encoder)) {

    if (ui == SPORT_UI_STATE_SELECT_SPORT) {
      ESP_LOGD(TAG, "Rotary click → SPORT CONFIRM (enter Level 2)");
      return INPUT_ACTION_SPORT_CONFIRM;
    }

    if (ui == SPORT_UI_STATE_SELECT_VARIANT) {
      ESP_LOGD(TAG, "Rotary click → CONFIRM SPORT (default variant)");
      return INPUT_ACTION_SPORT_CONFIRM;
    }

    ESP_LOGD(TAG, "Rotary button click in RUN MODE (ignored)");
  }

  return action;
}
