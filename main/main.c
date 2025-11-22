#include "../include/radio_comm.h"
#include "../include/button_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "CONTROLLER";

static RadioComm radio;
static Button control_button;
static uint8_t sequence = 0;
static uint16_t current_seconds = 0;
static bool is_running = false;

void app_main(void) {
    ESP_LOGI(TAG, "Starting Controller Application");

    // Initialize button
    button_begin(&control_button, CONTROL_BUTTON_PIN);

    // Initialize radio
    if (!radio_begin(&radio, NRF24_CE_PIN, NRF24_CSN_PIN)) {
        ESP_LOGE(TAG, "Failed to initialize radio");
        return;
    }

    // Dump radio registers for debugging
    radio_dump_registers(&radio);

    ESP_LOGI(TAG, "Controller initialized successfully");

    // Static variables for button state tracking
    static uint32_t last_time = 0;
    static uint32_t last_transmit = 0;
    static uint32_t last_gpio_debug = 0;
    static bool reset_button_pressed = false;
    static uint32_t reset_press_time = 0;
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
            ESP_LOGI(TAG, "CONTROL button state changed: %d -> %d", last_control_state, control_button.state);
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
        
        // Check for hold (2 seconds) - reset timer
        if (control_button_pressed && now_pressed && 
            (current_time - control_press_time >= 2000)) {
            ESP_LOGI(TAG, "CONTROL button held - resetting timer");
            is_running = false;
            current_seconds = 0;
            control_button_pressed = false; // Prevent repeat
        }
        
        // Check for release (short press) - toggle start/stop
        if (control_button_pressed && !now_pressed) {
            if (current_time - control_press_time < 2000) {
                ESP_LOGI(TAG, "CONTROL button released - toggling timer (running: %d)", !is_running);
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
            ESP_LOGI(TAG, "Timer state - running: %d, seconds: %d", is_running, current_seconds);
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