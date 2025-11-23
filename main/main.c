#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../include/button_driver.h"
#include "../include/radio_comm.h"
#include "../../radio-common/include/radio_config.h"
#include "../../sport_selector/include/sport_selector.h"

static const char *TAG = "CONTROLLER";

// Pin definitions (from AGENTS.md)
#define STATUS_LED_PIN GPIO_NUM_17
#define CONTROL_BUTTON_PIN GPIO_NUM_0
#define NRF24_CE_PIN GPIO_NUM_5
#define NRF24_CSN_PIN GPIO_NUM_4

static RadioComm radio;
static Button control_button;
static uint8_t sequence = 0;
static uint16_t current_seconds = 0;
static bool is_running = false;
static sport_config_t current_sport = {0};

// Function to set sport by type
void set_sport(sport_type_t sport_type) {
    current_sport = get_sport_config(sport_type);
    current_seconds = current_sport.play_clock_seconds;
    is_running = false; // Stop timer when changing sport
    
    ESP_LOGI(TAG, "Sport set to: %s %s (%d seconds)", 
             current_sport.name, current_sport.variation, current_sport.play_clock_seconds);
}

// Available sport types for selection:
// SPORT_BASKETBALL_24_SEC, SPORT_BASKETBALL_30_SEC
// SPORT_FOOTBALL_40_SEC, SPORT_FOOTBALL_25_SEC  
// SPORT_BASEBALL_15_SEC, SPORT_BASEBALL_20_SEC, SPORT_BASEBALL_14_SEC, SPORT_BASEBALL_19_SEC
// SPORT_VOLLEYBALL_8_SEC, SPORT_LACROSSE_30_SEC
// Custom sports: use get_custom_config(seconds, behavior)

void app_main(void) {
  ESP_LOGI(TAG, "Starting Controller Application");

  // Initialize sport configuration (default to basketball 24 sec)
  current_sport = get_sport_config(SPORT_BASKETBALL_24_SEC);
  current_seconds = current_sport.play_clock_seconds;
  ESP_LOGI(TAG, "Sport initialized: %s %s (%d seconds)", 
           current_sport.name, current_sport.variation, current_sport.play_clock_seconds);

  // Initialize button
  button_begin(&control_button, CONTROL_BUTTON_PIN);

  // Initialize radio
  if (!radio_begin(&radio, NRF24_CE_PIN, NRF24_CSN_PIN)) {
    ESP_LOGE(TAG, "Failed to initialize radio");
    return;
  }

  // Power up the radio and set to TX mode
  nrf24_power_up(&radio.base);
  nrf24_write_register(&radio.base, NRF24_REG_CONFIG, RADIO_CONFIG_TX_MODE);
  vTaskDelay(pdMS_TO_TICKS(2)); // Small delay to ensure mode switch

  ESP_LOGI(TAG, "Controller initialized successfully");

  // Static variables for button state tracking
  static uint32_t last_time = 0;
  static uint32_t last_transmit = 0;
  static uint32_t last_gpio_debug = 0;
  // static bool reset_button_pressed = false;
  // static uint32_t reset_press_time = 0;
  static bool control_button_pressed = false;
  static uint32_t control_press_time = 0;
  static ButtonState last_control_state = BUTTON_RELEASED;

  // Main loop
  while (1) {
    // Update button state
    button_update(&control_button);

    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

    // Log raw GPIO levels for debugging
    if (current_time - last_gpio_debug > 1000) {
      ESP_LOGI(TAG, "GPIO level - Control: %d | Button state - Control: %d",
               gpio_get_level(CONTROL_BUTTON_PIN),
               button_is_pressed(&control_button));
      last_gpio_debug = current_time;
    }

    // Check for button state change
    if (control_button.state != last_control_state) {
      ESP_LOGI(TAG, "CONTROL button state changed: %d -> %d",
               last_control_state, control_button.state);
      last_control_state = control_button.state;
    }

    // Handle control button - single button for start/stop/reset
    static bool was_pressed = false;
    bool now_pressed = button_is_pressed(&control_button);

    // Detect button press (rising edge)
    if (now_pressed && !was_pressed) {
      control_button_pressed = true;
      control_press_time = current_time;
      ESP_LOGI(TAG, "CONTROL button pressed");
    }

    // Check for hold (2 seconds) - reset timer to sport default
    if (control_button_pressed && now_pressed &&
        (current_time - control_press_time >= 2000)) {
      ESP_LOGI(TAG, "CONTROL button held - resetting timer to sport default");
      is_running = false;
      current_seconds = current_sport.play_clock_seconds;
      control_button_pressed = false; // Prevent repeat
    }

    // Check for release (short press) - toggle start/stop
    if (control_button_pressed && !now_pressed) {
      if (current_time - control_press_time < 2000) {
        ESP_LOGI(TAG, "CONTROL button released - toggling timer (running: %d)",
                 !is_running);
        is_running = !is_running;
      }
      control_button_pressed = false;
    }

    was_pressed = now_pressed;

    // Update time if running
    if (is_running && (current_time - last_time >= 1000)) {
      current_seconds++;
      last_time = current_time;
      ESP_LOGI(TAG, "Timer incrementing: %d seconds", current_seconds);
    }

    // Debug: log timer state every 5 seconds
    static uint32_t last_debug = 0;
    if (current_time - last_debug >= 5000) {
      ESP_LOGI(TAG, "Timer state - running: %d, seconds: %d", is_running,
               current_seconds);
      last_debug = current_time;
    }

    // Send time update every 250ms to keep time actual
    if (current_time - last_transmit >= 250) {
      radio_send_time(&radio, current_seconds, sequence++);
      last_transmit = current_time;
    }

    // Update link status LED and logging
    radio_update_link_status(&radio);

    vTaskDelay(pdMS_TO_TICKS(50)); // 20Hz update rate
  }
}
