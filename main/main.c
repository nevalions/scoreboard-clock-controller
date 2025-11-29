#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../include/button_driver.h"
#include "../include/lcd_i2c.h"
#include "../include/radio_comm.h"
#include "../include/rotary_encoder.h"
#include "../../radio-common/include/radio_config.h"
#include "../../sport_selector/include/colors.h"
#include "../../sport_selector/include/sport_selector.h"

static const char *TAG = "CONTROLLER";

// Pin definitions (from AGENTS.md)
#define STATUS_LED_PIN GPIO_NUM_17
#define CONTROL_BUTTON_PIN GPIO_NUM_0
#define NRF24_CE_PIN GPIO_NUM_5
#define NRF24_CSN_PIN GPIO_NUM_4

// Timing constants
#define DEBOUNCE_DELAY_MS 50
#define ROTARY_RESET_DELAY_MS 100
#define BUTTON_HOLD_RESET_MS 2000
#define BUTTON_DOUBLE_TAP_WINDOW_MS 500
#define TIMER_UPDATE_INTERVAL_MS 1000
#define RADIO_TRANSMIT_INTERVAL_MS 250
#define ZERO_CLEAR_DELAY_MS 3000
#define LINK_SUCCESS_WINDOW_MS 5000
#define LINK_FAILURE_WINDOW_MS 2000
#define MAIN_LOOP_DELAY_MS 50
#define GPIO_DEBUG_INTERVAL_MS 1000
#define TIMER_DEBUG_INTERVAL_MS 5000
#define LINK_STATUS_LOG_INTERVAL_MS 10000
#define RADIO_TRANSMIT_TIMEOUT_MS 20
#define RADIO_MODE_SWITCH_DELAY_MS 2

static RadioComm radio;
static Button control_button;
static RotaryEncoder rotary_encoder;
static LcdI2C lcd;
static uint8_t sequence = 0;
static uint16_t current_seconds = 0;
static bool is_running = false;
static sport_config_t current_sport = {0};
static bool null_sent = false;

// Helper functions
static inline uint32_t get_current_time_ms(void) {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

static inline bool time_elapsed(uint32_t start, uint32_t interval) {
    return (get_current_time_ms() - start) >= interval;
}

// Update LCD display with current sport and time
static void update_lcd_display(void) {
    lcd_i2c_clear(&lcd);
    lcd_i2c_set_cursor(&lcd, 0, 0);
    lcd_i2c_printf(&lcd, "%s %s", current_sport.name, current_sport.variation);
    lcd_i2c_set_cursor(&lcd, 0, 1);
    lcd_i2c_printf(&lcd, "Time: %03d", current_seconds);
}

static sport_type_t get_next_sport(sport_type_t current) {
  switch (current) {
  case SPORT_BASKETBALL_24_SEC:
    return SPORT_BASKETBALL_30_SEC;
  case SPORT_BASKETBALL_30_SEC:
    return SPORT_FOOTBALL_40_SEC;
  case SPORT_FOOTBALL_40_SEC:
    return SPORT_FOOTBALL_25_SEC;
  case SPORT_FOOTBALL_25_SEC:
    return SPORT_BASEBALL_15_SEC;
  case SPORT_BASEBALL_15_SEC:
    return SPORT_BASEBALL_20_SEC;
  case SPORT_BASEBALL_20_SEC:
    return SPORT_BASEBALL_14_SEC;
  case SPORT_BASEBALL_14_SEC:
    return SPORT_BASEBALL_19_SEC;
  case SPORT_BASEBALL_19_SEC:
    return SPORT_VOLLEYBALL_8_SEC;
  case SPORT_VOLLEYBALL_8_SEC:
    return SPORT_LACROSSE_30_SEC;
  case SPORT_LACROSSE_30_SEC:
    return SPORT_BASKETBALL_24_SEC;
  default:
    return SPORT_BASKETBALL_24_SEC;
  }
}

static sport_type_t get_prev_sport(sport_type_t current) {
  switch (current) {
  case SPORT_BASKETBALL_24_SEC:
    return SPORT_LACROSSE_30_SEC;
  case SPORT_BASKETBALL_30_SEC:
    return SPORT_BASKETBALL_24_SEC;
  case SPORT_FOOTBALL_40_SEC:
    return SPORT_BASKETBALL_30_SEC;
  case SPORT_FOOTBALL_25_SEC:
    return SPORT_FOOTBALL_40_SEC;
  case SPORT_BASEBALL_15_SEC:
    return SPORT_FOOTBALL_25_SEC;
  case SPORT_BASEBALL_20_SEC:
    return SPORT_BASEBALL_15_SEC;
  case SPORT_BASEBALL_14_SEC:
    return SPORT_BASEBALL_20_SEC;
  case SPORT_BASEBALL_19_SEC:
    return SPORT_BASEBALL_14_SEC;
  case SPORT_VOLLEYBALL_8_SEC:
    return SPORT_BASEBALL_19_SEC;
  case SPORT_LACROSSE_30_SEC:
    return SPORT_VOLLEYBALL_8_SEC;
  default:
    return SPORT_BASKETBALL_24_SEC;
  }
}

// Function to set sport by type
void set_sport(sport_type_t sport_type) {
  current_sport = get_sport_config(sport_type);
  current_seconds = current_sport.play_clock_seconds;
  is_running = false; // Stop timer when changing sport

  // Reset null tracking when sport changes
  null_sent = false;

  ESP_LOGI(TAG, "Sport set to: %s %s (%d seconds)", current_sport.name,
           current_sport.variation, current_sport.play_clock_seconds);
}

// Available sport types for selection:
// SPORT_BASKETBALL_24_SEC, SPORT_BASKETBALL_30_SEC
// SPORT_FOOTBALL_40_SEC, SPORT_FOOTBALL_25_SEC
// SPORT_BASEBALL_15_SEC, SPORT_BASEBALL_20_SEC, SPORT_BASEBALL_14_SEC,
// SPORT_BASEBALL_19_SEC SPORT_VOLLEYBALL_8_SEC, SPORT_LACROSSE_30_SEC Custom
// sports: use get_custom_config(seconds, behavior)

void app_main(void) {
  ESP_LOGI(TAG, "Starting Controller Application");

  // Initialize sport configuration (default to basketball 24 sec)
  current_sport = get_sport_config(SPORT_BASKETBALL_24_SEC);
  current_seconds = current_sport.play_clock_seconds;
  ESP_LOGI(TAG, "Sport initialized: %s %s (%d seconds)", current_sport.name,
           current_sport.variation, current_sport.play_clock_seconds);

  // Initialize button
  button_begin(&control_button, CONTROL_BUTTON_PIN);

  // Initialize rotary encoder
  rotary_encoder_begin(&rotary_encoder, ROTARY_CLK_PIN, ROTARY_DT_PIN,
                       ROTARY_SW_PIN);

  // Initialize I2C LCD
  ESP_LOGI(TAG, "Initializing LCD at address 0x%02X", LCD_I2C_ADDR);
  if (!lcd_i2c_begin(&lcd, LCD_I2C_ADDR, LCD_I2C_SDA_PIN, LCD_I2C_SCL_PIN)) {
    ESP_LOGE(TAG, "Failed to initialize I2C LCD");
    return;
  }

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

  // Initialize I2C LCD display
  update_lcd_display();

  // Static variables for button state tracking
  static uint32_t timer_last_update = 0;
  static uint32_t radio_last_transmit = 0;
  static uint32_t debug_last_gpio_output = 0;
  // static bool reset_button_pressed = false;
  // static uint32_t reset_press_time = 0;
  static bool control_button_active = false;
  static uint32_t control_button_press_start = 0;
  static ButtonState last_control_button_state = BUTTON_RELEASED;

  // Main loop
  while (1) {
    // Update button state
    button_update(&control_button);

    // Update rotary encoder
    rotary_encoder_update(&rotary_encoder);

    uint32_t current_time = get_current_time_ms();

    // Log raw GPIO levels for debugging
    if (time_elapsed(debug_last_gpio_output, GPIO_DEBUG_INTERVAL_MS)) {
      int sw_raw = gpio_get_level(ROTARY_SW_PIN);
      ESP_LOGI(TAG, "GPIO level - Control: %d | Button state - Control: %d | SW raw: %d",
               gpio_get_level(CONTROL_BUTTON_PIN),
               button_is_pressed(&control_button),
               sw_raw);
      ESP_LOGI("SWTEST", "SW raw level: %d", sw_raw);
      debug_last_gpio_output = current_time;
    }

    // Check for button state change
    if (control_button.state != last_control_button_state) {
      ESP_LOGI(TAG, "CONTROL button state changed: %d -> %d",
               last_control_button_state, control_button.state);
      last_control_button_state = control_button.state;
    }

    // Handle control button - single button for start/stop/reset with double
    // tap detection
    static bool was_pressed = false;
    static uint32_t last_press_time = 0;
    static uint32_t press_count = 0;
    bool now_pressed = button_is_pressed(&control_button);

    // Detect button press (rising edge)
    if (now_pressed && !was_pressed) {
      control_button_active = true;
      control_button_press_start = current_time;
      ESP_LOGI(TAG, "CONTROL button pressed");

      // Handle double tap detection
      press_count++;
      if (press_count == 1) {
        last_press_time = current_time;
        ESP_LOGI(TAG, "First press detected, count: %d", press_count);
      } else if (press_count == 2) {
        if ((current_time - last_press_time) <= BUTTON_DOUBLE_TAP_WINDOW_MS) {
          // Double tap detected - change sport
          static sport_type_t current_sport_type = SPORT_BASKETBALL_24_SEC;
          current_sport_type = get_next_sport(current_sport_type);
            set_sport(current_sport_type);
            update_lcd_display();

            ESP_LOGI(TAG, "Sport changed to: %s %s",
                     current_sport.name, current_sport.variation);
          press_count = 0; // Reset counter
        } else {
          // Too much time between presses, reset and count as first press
          last_press_time = current_time;
          press_count = 1;
          ESP_LOGI(TAG, "Press window expired, counting as first press");
        }
      }
    }

    // Reset double tap counter if window expires
    if (press_count > 0 &&
        (current_time - last_press_time) > BUTTON_DOUBLE_TAP_WINDOW_MS) {
      ESP_LOGI(TAG, "Double tap window expired, resetting count from %d",
               press_count);
      press_count = 0;
    }

    // Check for hold (2 seconds) - reset timer to sport default
    if (control_button_active && now_pressed &&
        time_elapsed(control_button_press_start, BUTTON_HOLD_RESET_MS)) {
      ESP_LOGI(TAG, "CONTROL button held - resetting timer to sport default");
      is_running = false;
      current_seconds = current_sport.play_clock_seconds;
      update_lcd_display();
      null_sent = false;             // Reset null flag on timer reset
      control_button_active = false; // Prevent repeat
      press_count = 0;               // Reset double tap counter on hold
    }

    // Check for release (short press) - toggle start/stop
    if (control_button_active && !now_pressed) {
      if (!time_elapsed(control_button_press_start, BUTTON_HOLD_RESET_MS)) {
        ESP_LOGI(TAG, "CONTROL button released - toggling timer (running: %d)",
                 !is_running);
        is_running = !is_running;
      }
      control_button_active = false;
    }

    was_pressed = now_pressed;

    //---------------------------------------------------------
    // ROTARY ENCODER SELECTION SYSTEM
    //---------------------------------------------------------

    static sport_type_t selected_sport_type = SPORT_BASKETBALL_24_SEC;
    static RotaryDirection last_action_dir = ROTARY_NONE;
    RotaryDirection dir = rotary_encoder_get_direction(&rotary_encoder);

    // 1. Handle rotation - only SELECT sport, don't change it
    if (dir != ROTARY_NONE && last_action_dir == ROTARY_NONE) {
      if (rotary_encoder_is_button_pressed(&rotary_encoder)) {
        // Button held while rotating → adjust time
        if (dir == ROTARY_CW && current_seconds < 999)
          current_seconds++;
        else if (dir == ROTARY_CCW && current_seconds > 0)
          current_seconds--;

        update_lcd_display();
        ESP_LOGI(TAG, "Time adjusted by rotary: %d", current_seconds);
      } else {
        // Rotation without button → navigate through sports (selection only)
        if (dir == ROTARY_CW)
          selected_sport_type = get_next_sport(selected_sport_type);
        else
          selected_sport_type = get_prev_sport(selected_sport_type);

        // Show selection on LCD (but don't change actual sport)
        sport_config_t selected_sport = get_sport_config(selected_sport_type);
        lcd_i2c_clear(&lcd);
        lcd_i2c_set_cursor(&lcd, 0, 0);
        lcd_i2c_printf(&lcd, ">%s %s", selected_sport.name, selected_sport.variation);
        lcd_i2c_set_cursor(&lcd, 0, 1);
        lcd_i2c_printf(&lcd, "Time: %03d", current_seconds);

        ESP_LOGI(TAG, "Sport SELECTED: %s %s (press to confirm)",
                 selected_sport.name, selected_sport.variation);
      }

      // Mark that we've taken action for this detent
      last_action_dir = dir;
    }

    // 2. Reset the state ONLY when encoder direction returns to NONE
    if (dir == ROTARY_NONE) {
      last_action_dir = ROTARY_NONE;
    }

    // 3. Handle rotary button press - CONFIRM selected sport or QUICK RESET
    if (rotary_encoder_get_button_press(&rotary_encoder)) {
      // Check if selected sport is different from current sport
      sport_config_t selected_sport = get_sport_config(selected_sport_type);
      if (selected_sport.play_clock_seconds != current_sport.play_clock_seconds ||
          strcmp(selected_sport.name, current_sport.name) != 0) {
        // Different sport selected - confirm and change
        set_sport(selected_sport_type);
        ESP_LOGI(TAG, "Rotary button: SPORT CONFIRMED → %s %s",
                 current_sport.name, current_sport.variation);
      } else {
        // Same sport selected - quick reset to sport default
        is_running = false;
        current_seconds = current_sport.play_clock_seconds;
        null_sent = false;  // Reset null flag

        ESP_LOGI(TAG, "Rotary button: QUICK RESET → %d sec",
                 current_seconds);
      }
      
      update_lcd_display();
    }

    // Update time if running
    static uint32_t zero_reached_timestamp = 0;
    if (is_running &&
        time_elapsed(timer_last_update, TIMER_UPDATE_INTERVAL_MS)) {
      if (current_seconds > 0) {
        current_seconds--;
        update_lcd_display();
        timer_last_update = current_time;
        ESP_LOGI(TAG, "Timer counting down: %d seconds", current_seconds);
      } else {
        // Timer reached zero, stop running and record time
        is_running = false;
        zero_reached_timestamp = current_time;
        null_sent = false; // Reset null flag when timer restarts
        ESP_LOGI(TAG, "Timer reached zero - stopped");
      }
    }

    // After 3 seconds at zero, send null to clear display
    if (!is_running && current_seconds == 0 && zero_reached_timestamp > 0 &&
        time_elapsed(zero_reached_timestamp, ZERO_CLEAR_DELAY_MS) &&
        !null_sent) {
      // Send null with deep red color
      radio_send_time(
          &radio, 0xFF, 255, 0, 0,
          sequence++);  // Send 0xFF as null indicator with deep red color
      null_sent = true; // Mark that null has been sent
      ESP_LOGI(TAG, "Sent null signal to clear display");
    }

    // Debug: log timer state every 5 seconds
    static uint32_t debug_last_timer_output = 0;
    if (time_elapsed(debug_last_timer_output, TIMER_DEBUG_INTERVAL_MS)) {
      ESP_LOGI(TAG, "Timer state - running: %d, seconds: %d", is_running,
               current_seconds);
      debug_last_timer_output = current_time;
    }

    // Send time update every 250ms to keep time actual
    if (time_elapsed(radio_last_transmit, RADIO_TRANSMIT_INTERVAL_MS)) {
      uint8_t time_to_send = current_seconds;

      // If we've sent null and are still at zero, keep sending null
      if (null_sent && !is_running && current_seconds == 0) {
        time_to_send = 0xFF; // Send null indicator
      }

      // Get color based on sport configuration and current time
      color_t color = get_sport_color(current_sport.color_scheme, time_to_send);
      uint8_t r = color.r, g = color.g, b = color.b;

      radio_send_time(&radio, time_to_send, r, g, b, sequence++);
      radio_last_transmit = current_time;
    }

    // Update link status LED and logging
    radio_update_link_status(&radio);

    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_MS)); // 20Hz update rate
  }
}
