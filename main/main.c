#include "../include/radio_comm.h"
#include "../include/button_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "CONTROLLER";

static RadioComm radio;
static Button start_button, stop_button, reset_button;
static uint8_t sequence = 0;
static uint16_t current_seconds = 0;
static bool is_running = false;

void app_main(void) {
    ESP_LOGI(TAG, "Starting Controller Application");

    // Initialize buttons
    button_begin(&start_button, START_BUTTON_PIN);
    button_begin(&stop_button, STOP_BUTTON_PIN);
    button_begin(&reset_button, RESET_BUTTON_PIN);

    // Initialize radio
    if (!radio_begin(&radio, NRF24_CE_PIN, NRF24_CSN_PIN)) {
        ESP_LOGE(TAG, "Failed to initialize radio");
        return;
    }

    // Dump radio registers for debugging
    radio_dump_registers(&radio);

    ESP_LOGI(TAG, "Controller initialized successfully");

    // Main loop
    while (1) {
        // Update button states
        button_update(&start_button);
        button_update(&stop_button);
        button_update(&reset_button);

        // Check for button presses
        if (button_get_falling_edge(&start_button)) {
            ESP_LOGI(TAG, "START button pressed");
            is_running = true;
            radio_send_command(&radio, CMD_RUN, current_seconds, sequence++);
        }

        if (button_get_falling_edge(&stop_button)) {
            ESP_LOGI(TAG, "STOP button pressed");
            is_running = false;
            radio_send_command(&radio, CMD_STOP, current_seconds, sequence++);
        }

        if (button_get_falling_edge(&reset_button)) {
            ESP_LOGI(TAG, "RESET button pressed");
            is_running = false;
            current_seconds = 0;
            radio_send_command(&radio, CMD_RESET, current_seconds, sequence++);
        }

        // Update time if running
        static uint32_t last_time = 0;
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (is_running && (current_time - last_time >= 1000)) {
            current_seconds++;
            last_time = current_time;
            
            // Send time update every 10 seconds
            if (current_seconds % 10 == 0) {
                radio_send_command(&radio, CMD_RUN, current_seconds, sequence++);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // 20Hz update rate
    }
}