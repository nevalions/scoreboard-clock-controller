#include "../../radio-common/include/radio_config.h"
#include "colors.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "input_handler.h"
#include "lcd_i2c.h"
#include "radio_comm.h"
#include "rotary_encoder.h"
#include "sport_manager.h"
#include "sport_selector.h"
#include "st7735_lcd.h"
#include "timer_manager.h"
#include "ui_manager.h"
#include <stdbool.h>
#include <stdint.h>

static const char *TAG = "CONTROLLER";

// -----------------------------------------------------------------------------
// Pin definitions
// -----------------------------------------------------------------------------
#define STATUS_LED_PIN GPIO_NUM_17
#define CONTROL_BUTTON_PIN GPIO_NUM_0
#define NRF24_CE_PIN GPIO_NUM_5
#define NRF24_CSN_PIN GPIO_NUM_4

#define ST7735_CS_PIN GPIO_NUM_27
#define ST7735_DC_PIN GPIO_NUM_26
#define ST7735_RST_PIN GPIO_NUM_25
#define ST7735_SDA_PIN GPIO_NUM_13
#define ST7735_SCL_PIN GPIO_NUM_14

// Rotary encoder pins
#define ROTARY_CLK_PIN GPIO_NUM_33
#define ROTARY_DT_PIN GPIO_NUM_16
#define ROTARY_SW_PIN GPIO_NUM_32

// External buttons
#define BTN_PRESET1_PIN GPIO_NUM_21
#define BTN_PRESET2_PIN GPIO_NUM_22
#define BTN_PRESET3_PIN GPIO_NUM_36
#define BTN_PRESET4_PIN GPIO_NUM_34
#define BTN_START_PIN GPIO_NUM_35
#define BTN_RESET_PIN GPIO_NUM_15

#define USE_ST7735_DISPLAY true

// Timing
#define RADIO_TRANSMIT_INTERVAL_MS 250
#define MAIN_LOOP_DELAY_MS 50

// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------
static RadioComm radio;
static uint8_t sequence = 0;

typedef struct {
  uint32_t radio_last_transmit;
} MainState;

static MainState main_state = {0};

// -----------------------------------------------------------------------------
// MAIN APPLICATION
// -----------------------------------------------------------------------------
void app_main(void) {

  ESP_LOGI(TAG, "Starting Controller Application");

  SportManager sport_mgr;
  TimerManager timer_mgr;
  UiManager ui_mgr;
  InputHandler input_handler;

  sport_manager_init(&sport_mgr);

  // Boot into sport menu
  sport_manager_enter_sport_menu(&sport_mgr);

  sport_config_t initial_sport = sport_manager_get_current_sport(&sport_mgr);
  timer_manager_init(&timer_mgr, initial_sport.play_clock_seconds);

  // Input handler
  input_handler_init(&input_handler, CONTROL_BUTTON_PIN, ROTARY_CLK_PIN,
                     ROTARY_DT_PIN, ROTARY_SW_PIN, BTN_PRESET1_PIN,
                     BTN_PRESET2_PIN, BTN_PRESET3_PIN, BTN_PRESET4_PIN,
                     BTN_START_PIN, BTN_RESET_PIN);

#if USE_ST7735_DISPLAY
  ui_manager_init_st7735(&ui_mgr, ST7735_CS_PIN, ST7735_DC_PIN, ST7735_RST_PIN,
                         ST7735_SDA_PIN, ST7735_SCL_PIN);
#else
  ui_manager_init_lcd_i2c(&ui_mgr, LCD_I2C_ADDR, LCD_I2C_SDA_PIN,
                          LCD_I2C_SCL_PIN);
#endif

  if (!ui_mgr.initialized) {
    ESP_LOGE(TAG, "Display init failed!");
    return;
  }

  // Initial sport menu draw
  {
    size_t group_count;
    const sport_group_t *groups = sport_manager_get_groups(&group_count);

    ui_manager_show_sport_menu(
        &ui_mgr, groups, group_count,
        sport_manager_get_current_group_index(&sport_mgr));
  }

  // Radio initialization
  if (!radio_begin(&radio, NRF24_CE_PIN, NRF24_CSN_PIN)) {
    ESP_LOGE(TAG, "Radio init failed");
    return;
  }

  nrf24_power_up(&radio.base);
  nrf24_write_register(&radio.base, NRF24_REG_CONFIG, RADIO_CONFIG_TX_MODE);

  ESP_LOGI(TAG, "Controller initialized");

  uint16_t last_time = 65535;

  // -------------------------------------------------------------------------
  // MAIN LOOP
  // -------------------------------------------------------------------------
  while (1) {

    InputAction action =
        input_handler_update(&input_handler, &sport_mgr, &timer_mgr);

    sport_ui_state_t ui_state = sport_manager_get_ui_state(&sport_mgr);
    sport_config_t current_sport = sport_manager_get_current_sport(&sport_mgr);

    if (action != INPUT_ACTION_NONE) {
      ESP_LOGI(TAG, "MAIN: got action=%d in ui_state=%d", action, ui_state);
    }

    // =====================================================================
    // INPUT HANDLING
    // =====================================================================
    switch (action) {

    // *********************************************************************
    // START / STOP TOGGLE
    // *********************************************************************
    case INPUT_ACTION_START_STOP:
      if (ui_state == SPORT_UI_STATE_RUNNING)
        timer_manager_start_stop(&timer_mgr);
      break;

    // *********************************************************************
    // RESET — ALWAYS STOP THEN RESET
    // *********************************************************************
    case INPUT_ACTION_RESET:
      if (ui_state == SPORT_UI_STATE_RUNNING) {

        timer_manager_stop(&timer_mgr);
        timer_manager_reset(&timer_mgr, current_sport.play_clock_seconds);

        ui_manager_update_display(&ui_mgr, &current_sport,
                                  timer_manager_get_seconds(&timer_mgr),
                                  &sport_mgr);
      }
      break;

    // *********************************************************************
    // TIME ADJUST
    // *********************************************************************
    case INPUT_ACTION_TIME_ADJUST:
      if (ui_state == SPORT_UI_STATE_RUNNING) {
        ui_manager_update_time(&ui_mgr, &current_sport,
                               timer_manager_get_seconds(&timer_mgr),
                               &sport_mgr);
      }
      break;

    // *********************************************************************
    // PRESET BUTTONS — STOP TIMER + LOAD PRESET
    // *********************************************************************
    case INPUT_ACTION_PRESET_1:
    case INPUT_ACTION_PRESET_2:
    case INPUT_ACTION_PRESET_3:
    case INPUT_ACTION_PRESET_4: {

      uint8_t idx = action - INPUT_ACTION_PRESET_1;
      const sport_group_t *group = sport_manager_get_current_group(&sport_mgr);

      ESP_LOGW(TAG, "Preset %d", idx + 1);

      if (group && idx < group->variant_count) {

        timer_manager_stop(&timer_mgr);

        sport_type_t t = group->variants[idx];
        sport_manager_set_sport(&sport_mgr, t);

        sport_manager_exit_menu(&sport_mgr);
        current_sport = sport_manager_get_current_sport(&sport_mgr);

        timer_manager_reset(&timer_mgr, current_sport.play_clock_seconds);

        ui_manager_update_display(&ui_mgr, &current_sport,
                                  timer_manager_get_seconds(&timer_mgr),
                                  &sport_mgr);
      }

    } break;

    // *********************************************************************
    // TOGGLE SPORT MENU — STOP TIMER BEFORE ENTER
    // *********************************************************************
    case INPUT_ACTION_SPORT_SELECT:

      ui_state = sport_manager_get_ui_state(&sport_mgr);

      if (ui_state == SPORT_UI_STATE_RUNNING) {

        timer_manager_stop(&timer_mgr);

        sport_manager_enter_sport_menu(&sport_mgr);

        size_t group_count;
        const sport_group_t *groups = sport_manager_get_groups(&group_count);

        ui_manager_show_sport_menu(
            &ui_mgr, groups, group_count,
            sport_manager_get_current_group_index(&sport_mgr));

      } else {

        sport_manager_exit_menu(&sport_mgr);
        current_sport = sport_manager_get_current_sport(&sport_mgr);

        timer_manager_stop(&timer_mgr);
        timer_manager_reset(&timer_mgr, current_sport.play_clock_seconds);

        ui_manager_update_display(&ui_mgr, &current_sport,
                                  timer_manager_get_seconds(&timer_mgr),
                                  &sport_mgr);
      }
      break;

    // *********************************************************************
    // ROTARY SPORT SCROLL
    // *********************************************************************
    case INPUT_ACTION_SPORT_NEXT:
    case INPUT_ACTION_SPORT_PREV:

      ui_state = sport_manager_get_ui_state(&sport_mgr);

      if (ui_state == SPORT_UI_STATE_RUNNING)
        break;

      size_t group_count;
      const sport_group_t *groups = sport_manager_get_groups(&group_count);

      if (ui_state == SPORT_UI_STATE_SELECT_SPORT) {

        int current = sport_manager_get_current_group_index(&sport_mgr);
        int target = (action == INPUT_ACTION_SPORT_NEXT)
                         ? (current + 1) % group_count
                         : (current == 0 ? group_count - 1 : current - 1);

        while (sport_manager_get_current_group_index(&sport_mgr) != target)
          sport_manager_next_sport(&sport_mgr);

        ui_st7735_update_sport_menu_selection(
            &ui_mgr, groups, group_count,
            sport_manager_get_current_group_index(&sport_mgr));
      }

      break;

    // *********************************************************************
    // CONFIRM SELECTION — STOP TIMER BEFORE APPLYING
    // *********************************************************************
    case INPUT_ACTION_SPORT_CONFIRM:

      ui_state = sport_manager_get_ui_state(&sport_mgr);

      if (ui_state == SPORT_UI_STATE_SELECT_SPORT) {

        sport_manager_enter_variant_menu(&sport_mgr);
        const sport_group_t *group =
            sport_manager_get_current_group(&sport_mgr);

        ui_manager_show_variant_menu(&ui_mgr, group);
      }

      else if (ui_state == SPORT_UI_STATE_SELECT_VARIANT) {

        timer_manager_stop(&timer_mgr);

        sport_manager_confirm_selection(&sport_mgr);
        current_sport = sport_manager_get_current_sport(&sport_mgr);

        timer_manager_reset(&timer_mgr, current_sport.play_clock_seconds);

        ui_manager_update_display(&ui_mgr, &current_sport,
                                  timer_manager_get_seconds(&timer_mgr),
                                  &sport_mgr);
      }
      break;

    default:
      break;
    }

    // =====================================================================
    // TIMER UPDATE
    // =====================================================================
    timer_manager_update(&timer_mgr);

    uint16_t now = timer_manager_get_seconds(&timer_mgr);

    if (now != last_time &&
        sport_manager_get_ui_state(&sport_mgr) == SPORT_UI_STATE_RUNNING) {

      ui_manager_update_time(&ui_mgr, &current_sport, now, &sport_mgr);
      last_time = now;
    }

    // =====================================================================
    // RADIO UPDATE
    // =====================================================================
    uint32_t t = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (t - main_state.radio_last_transmit >= RADIO_TRANSMIT_INTERVAL_MS) {

      uint8_t sec = timer_manager_get_seconds(&timer_mgr);
      color_t c = get_sport_color(current_sport.color_scheme, sec);

      radio_send_time(&radio, sec, c.r, c.g, c.b, sequence++);

      main_state.radio_last_transmit = t;
    }

    radio_update_link_status(&radio);

    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_MS));
  }
}
