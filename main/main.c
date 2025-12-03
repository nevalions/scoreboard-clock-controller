#include "../../radio-common/include/radio_config.h"
#include "../../sport_selector/include/colors.h"
#include "../../sport_selector/include/sport_selector.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "input_handler.h"
#include "lcd_i2c.h"
#include "radio_comm.h"
#include "rotary_encoder.h"
#include "sport_manager.h"
#include "st7735_lcd.h"
#include "timer_manager.h"
#include "ui_manager.h"
#include <stdbool.h>
#include <stdint.h>

static const char *TAG = "CONTROLLER";

#define STATUS_LED_PIN GPIO_NUM_17
#define CONTROL_BUTTON_PIN GPIO_NUM_0
#define NRF24_CE_PIN GPIO_NUM_5
#define NRF24_CSN_PIN GPIO_NUM_4

// ST7735 display pins
#define ST7735_CS_PIN GPIO_NUM_27
#define ST7735_DC_PIN GPIO_NUM_26
#define ST7735_RST_PIN GPIO_NUM_25
#define ST7735_SDA_PIN GPIO_NUM_13
#define ST7735_SCL_PIN GPIO_NUM_14

#define USE_ST7735_DISPLAY true

#define RADIO_TRANSMIT_INTERVAL_MS 250
#define MAIN_LOOP_DELAY_MS 50
#define TIMER_DEBUG_INTERVAL_MS 5000
#define LINK_STATUS_LOG_INTERVAL_MS 10000
#define RADIO_MODE_SWITCH_DELAY_MS 2

static RadioComm radio;
static uint8_t sequence = 0;

typedef struct {
  uint32_t radio_last_transmit;
  uint32_t debug_last_timer_output;
} MainState;

static MainState main_state = {0};

void app_main(void) {
  ESP_LOGI(TAG, "Starting Controller Application");

  SportManager sport_mgr;
  TimerManager timer_mgr;
  UiManager ui_mgr;
  InputHandler input_handler;

  sport_manager_init(&sport_mgr);

  sport_config_t initial_sport = sport_manager_get_current_sport(&sport_mgr);
  timer_manager_init(&timer_mgr, initial_sport.play_clock_seconds);

  input_handler_init(&input_handler, CONTROL_BUTTON_PIN, ROTARY_CLK_PIN,
                     ROTARY_DT_PIN, ROTARY_SW_PIN);

#if USE_ST7735_DISPLAY
  ESP_LOGI(TAG, "Initializing ST7735 Display");
  vTaskDelay(pdMS_TO_TICKS(100));
  ui_manager_init_st7735(&ui_mgr, ST7735_CS_PIN, ST7735_DC_PIN, ST7735_RST_PIN,
                         ST7735_SDA_PIN, ST7735_SCL_PIN);
  if (!ui_mgr.initialized) {
    ESP_LOGE(TAG, "Failed to initialize ST7735");
    return;
  }

  vTaskDelay(pdMS_TO_TICKS(100));
  st7735_test_pattern(&ui_mgr.st7735);
  vTaskDelay(pdMS_TO_TICKS(3000));
  ui_manager_clear(&ui_mgr);

#else
  ESP_LOGI(TAG, "Initializing I2C LCD");
  ui_manager_init_lcd_i2c(&ui_mgr, LCD_I2C_ADDR, LCD_I2C_SDA_PIN,
                          LCD_I2C_SCL_PIN);
#endif

  if (!ui_mgr.initialized) {
    ESP_LOGE(TAG, "Display init failed");
    return;
  }

  if (!radio_begin(&radio, NRF24_CE_PIN, NRF24_CSN_PIN)) {
    ESP_LOGE(TAG, "Radio init failed");
    return;
  }

  nrf24_power_up(&radio.base);
  nrf24_write_register(&radio.base, NRF24_REG_CONFIG, RADIO_CONFIG_TX_MODE);
  vTaskDelay(pdMS_TO_TICKS(RADIO_MODE_SWITCH_DELAY_MS));

  ESP_LOGI(TAG, "Controller initialized");

  ui_manager_update_display(&ui_mgr, &initial_sport,
                            timer_manager_get_seconds(&timer_mgr));

  while (1) {

    InputAction action =
        input_handler_update(&input_handler, &sport_mgr, &timer_mgr);

    sport_config_t current_sport = sport_manager_get_current_sport(&sport_mgr);

    switch (action) {

    case INPUT_ACTION_SPORT_CHANGE:
    case INPUT_ACTION_SPORT_CONFIRM:
      timer_manager_reset(&timer_mgr, current_sport.play_clock_seconds);
      ui_manager_update_display(&ui_mgr, &current_sport,
                                timer_manager_get_seconds(&timer_mgr));
      break;

    case INPUT_ACTION_RESET:
    case INPUT_ACTION_TIME_ADJUST:
    case INPUT_ACTION_START_STOP:
      ui_manager_update_display(&ui_mgr, &current_sport,
                                timer_manager_get_seconds(&timer_mgr));
      break;

    case INPUT_ACTION_SPORT_SELECT: {
      sport_config_t sel =
          get_sport_config(sport_manager_get_selected_type(&sport_mgr));
      ui_manager_show_sport_selection(&ui_mgr, &sel,
                                      timer_manager_get_seconds(&timer_mgr));
    } break;

    default:
      break;
    }

    timer_manager_update(&timer_mgr);

    // ===============================================================
    //      *TIME-ONLY UPDATE* — NO FULL REDRAW, ONLY TIMER TEXT
    // ===============================================================
    static uint16_t last_displayed_time = 65535;
    uint16_t now = timer_manager_get_seconds(&timer_mgr);

    if (now != last_displayed_time) {
      ui_manager_update_time(&ui_mgr, &current_sport, now);
      last_displayed_time = now;
    }

    // ===============================================================
    //                      RADIO SENDING
    // ===============================================================
    uint32_t t = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (t - main_state.radio_last_transmit >= RADIO_TRANSMIT_INTERVAL_MS) {

      uint8_t time_to_send = timer_manager_get_seconds(&timer_mgr);
      color_t c = get_sport_color(current_sport.color_scheme, time_to_send);

      radio_send_time(&radio, time_to_send, c.r, c.g, c.b, sequence++);

      main_state.radio_last_transmit = t;
    }

    radio_update_link_status(&radio);
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_MS));
  }
}
