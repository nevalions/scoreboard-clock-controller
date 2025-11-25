#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../include/button_driver.h"
#include "../include/radio_comm.h"
#include "../include/lcd_i2c.h"
#include "../include/rotary_encoder.h"
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
static RotaryEncoder rotary_encoder;
static LcdI2C lcd;
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

  // Initialize rotary encoder
  rotary_encoder_begin(&rotary_encoder, ROTARY_CLK_PIN, ROTARY_DT_PIN, ROTARY_SW_PIN);

  // Initialize I2C LCD
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
  lcd_i2c_clear(&lcd);
  lcd_i2c_set_cursor(&lcd, 0, 0);
  lcd_i2c_printf(&lcd, "%s %s", current_sport.name, current_sport.variation);
  lcd_i2c_set_cursor(&lcd, 0, 1);
  lcd_i2c_printf(&lcd, "Time: %03d", current_seconds);

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

    // Update rotary encoder
    rotary_encoder_update(&rotary_encoder);

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

    // Handle rotary encoder for sport/time adjustment
    RotaryDirection direction = rotary_encoder_get_direction(&rotary_encoder);
    if (direction != ROTARY_NONE) {
        if (rotary_encoder_is_button_pressed(&rotary_encoder)) {
            // Button pressed + rotation = adjust time
            if (direction == ROTARY_CW && current_seconds < 999) {
                current_seconds++;
            } else if (direction == ROTARY_CCW && current_seconds > 0) {
                current_seconds--;
            }
            ESP_LOGI(TAG, "Time adjusted: %d seconds", current_seconds);
        } else {
            // Rotation only = cycle through sports
            static sport_type_t current_sport_type = SPORT_BASKETBALL_24_SEC;
            
            if (direction == ROTARY_CW) {
                // Cycle to next sport
                switch (current_sport_type) {
                    case SPORT_BASKETBALL_24_SEC: current_sport_type = SPORT_BASKETBALL_30_SEC; break;
                    case SPORT_BASKETBALL_30_SEC: current_sport_type = SPORT_FOOTBALL_40_SEC; break;
                    case SPORT_FOOTBALL_40_SEC: current_sport_type = SPORT_FOOTBALL_25_SEC; break;
                    case SPORT_FOOTBALL_25_SEC: current_sport_type = SPORT_BASEBALL_15_SEC; break;
                    case SPORT_BASEBALL_15_SEC: current_sport_type = SPORT_BASEBALL_20_SEC; break;
                    case SPORT_BASEBALL_20_SEC: current_sport_type = SPORT_BASEBALL_14_SEC; break;
                    case SPORT_BASEBALL_14_SEC: current_sport_type = SPORT_BASEBALL_19_SEC; break;
                    case SPORT_BASEBALL_19_SEC: current_sport_type = SPORT_VOLLEYBALL_8_SEC; break;
                    case SPORT_VOLLEYBALL_8_SEC: current_sport_type = SPORT_LACROSSE_30_SEC; break;
                    case SPORT_LACROSSE_30_SEC: current_sport_type = SPORT_BASKETBALL_24_SEC; break;
                    default: current_sport_type = SPORT_BASKETBALL_24_SEC; break;
                }
            } else if (direction == ROTARY_CCW) {
                // Cycle to previous sport
                switch (current_sport_type) {
                    case SPORT_BASKETBALL_24_SEC: current_sport_type = SPORT_LACROSSE_30_SEC; break;
                    case SPORT_BASKETBALL_30_SEC: current_sport_type = SPORT_BASKETBALL_24_SEC; break;
                    case SPORT_FOOTBALL_40_SEC: current_sport_type = SPORT_BASKETBALL_30_SEC; break;
                    case SPORT_FOOTBALL_25_SEC: current_sport_type = SPORT_FOOTBALL_40_SEC; break;
                    case SPORT_BASEBALL_15_SEC: current_sport_type = SPORT_FOOTBALL_25_SEC; break;
                    case SPORT_BASEBALL_20_SEC: current_sport_type = SPORT_BASEBALL_15_SEC; break;
                    case SPORT_BASEBALL_14_SEC: current_sport_type = SPORT_BASEBALL_20_SEC; break;
                    case SPORT_BASEBALL_19_SEC: current_sport_type = SPORT_BASEBALL_14_SEC; break;
                    case SPORT_VOLLEYBALL_8_SEC: current_sport_type = SPORT_BASEBALL_19_SEC; break;
                    case SPORT_LACROSSE_30_SEC: current_sport_type = SPORT_VOLLEYBALL_8_SEC; break;
                    default: current_sport_type = SPORT_BASKETBALL_24_SEC; break;
                }
            }
            
            set_sport(current_sport_type);
        }
    }

    // Handle rotary encoder button press (standalone)
    static bool last_rotary_button = false;
    bool current_rotary_button = rotary_encoder_get_button_press(&rotary_encoder);
    if (current_rotary_button && !last_rotary_button) {
        ESP_LOGI(TAG, "Rotary encoder button pressed - quick reset to current sport default");
        is_running = false;
        current_seconds = current_sport.play_clock_seconds;
    }
    last_rotary_button = current_rotary_button;

    // Update time if running
    static uint32_t zero_reached_time = 0;
    if (is_running && (current_time - last_time >= 1000)) {
      if (current_seconds > 0) {
        current_seconds--;
        last_time = current_time;
        ESP_LOGI(TAG, "Timer counting down: %d seconds", current_seconds);
      } else {
        // Timer reached zero, stop running and record time
        is_running = false;
        zero_reached_time = current_time;
        ESP_LOGI(TAG, "Timer reached zero - stopped");
      }
    }

    // After 3 seconds at zero, send null to clear display
    if (!is_running && current_seconds == 0 && zero_reached_time > 0 && 
        (current_time - zero_reached_time >= 3000)) {
      radio_send_time(&radio, 0xFF, sequence++); // Send 0xFF as null indicator
      zero_reached_time = 0; // Prevent repeated sends
      ESP_LOGI(TAG, "Sent null signal to clear display");
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
      uint8_t time_to_send = current_seconds;
      
      // If we've sent null and are still at zero, keep sending null
      if (!is_running && current_seconds == 0 && zero_reached_time > 0 && 
          (current_time - zero_reached_time >= 3000)) {
        time_to_send = 0xFF; // Send null indicator
      }
      
      radio_send_time(&radio, time_to_send, sequence++);
      last_transmit = current_time;
    }

    // Update link status LED and logging
    radio_update_link_status(&radio);

    vTaskDelay(pdMS_TO_TICKS(50)); // 20Hz update rate
  }
}
