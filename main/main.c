#include "../../radio-common/include/radio_config.h"
#include "../../sport_selector/include/colors.h"
#include "../../sport_selector/include/sport_selector.h"

#include "../include/input_handler.h"
#include "../include/lcd_i2c.h"
#include "../include/radio_comm.h"
#include "../include/rotary_encoder.h"
#include "../include/sport_manager.h"
#include "../include/timer_manager.h"
#include "../include/ui_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "CONTROLLER";

#define STATUS_LED_PIN GPIO_NUM_17
#define CONTROL_BUTTON_PIN GPIO_NUM_0
#define NRF24_CE_PIN GPIO_NUM_5
#define NRF24_CSN_PIN GPIO_NUM_4

#define RADIO_TRANSMIT_INTERVAL_MS 250
#define MAIN_LOOP_DELAY_MS 50
#define TIMER_DEBUG_INTERVAL_MS 5000
#define LINK_STATUS_LOG_INTERVAL_MS 10000
#define RADIO_MODE_SWITCH_DELAY_MS 2

static RadioComm radio;
static uint8_t sequence = 0;

static inline uint32_t get_current_time_ms(void) {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

static inline bool time_elapsed(uint32_t start, uint32_t interval) {
    return (get_current_time_ms() - start) >= interval;
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting Controller Application");

    SportManager sport_mgr;
    TimerManager timer_mgr;
    UiManager ui_mgr;
    InputHandler input_handler;

    sport_manager_init(&sport_mgr);
    
    sport_config_t initial_sport = sport_manager_get_current_sport(&sport_mgr);
    timer_manager_init(&timer_mgr, initial_sport.play_clock_seconds);
    
    ESP_LOGI(TAG, "Sport initialized: %s %s (%d seconds)", 
             initial_sport.name, initial_sport.variation, initial_sport.play_clock_seconds);

    input_handler_init(&input_handler, CONTROL_BUTTON_PIN, ROTARY_CLK_PIN, ROTARY_DT_PIN, ROTARY_SW_PIN);

    ui_manager_init(&ui_mgr, LCD_I2C_ADDR, LCD_I2C_SDA_PIN, LCD_I2C_SCL_PIN);
    if (!ui_mgr.lcd.initialized) {
        ESP_LOGE(TAG, "Failed to initialize I2C LCD");
        return;
    }

    // ui_manager_run_lcd_tests(&ui_mgr);  // Commented out for faster startup

    if (!radio_begin(&radio, NRF24_CE_PIN, NRF24_CSN_PIN)) {
        ESP_LOGE(TAG, "Failed to initialize radio");
        return;
    }

    nrf24_power_up(&radio.base);
    nrf24_write_register(&radio.base, NRF24_REG_CONFIG, RADIO_CONFIG_TX_MODE);
    vTaskDelay(pdMS_TO_TICKS(RADIO_MODE_SWITCH_DELAY_MS));

    ESP_LOGI(TAG, "Controller initialized successfully");

    ui_manager_update_display(&ui_mgr, &initial_sport, timer_manager_get_seconds(&timer_mgr));

    static uint32_t radio_last_transmit = 0;

    while (1) {
        InputAction action = input_handler_update(&input_handler, &sport_mgr, &timer_mgr);
        

        
        sport_config_t current_sport = sport_manager_get_current_sport(&sport_mgr);
        
        switch (action) {
            case INPUT_ACTION_SPORT_CHANGE:
            case INPUT_ACTION_SPORT_CONFIRM:
                current_sport = sport_manager_get_current_sport(&sport_mgr);
                timer_manager_reset(&timer_mgr, current_sport.play_clock_seconds);
                ui_manager_update_display(&ui_mgr, &current_sport, timer_manager_get_seconds(&timer_mgr));
                break;
                
            case INPUT_ACTION_RESET:
                ui_manager_update_display(&ui_mgr, &current_sport, timer_manager_get_seconds(&timer_mgr));
                break;
                
            case INPUT_ACTION_TIME_ADJUST:
                ui_manager_update_display(&ui_mgr, &current_sport, timer_manager_get_seconds(&timer_mgr));
                break;
                
            case INPUT_ACTION_SPORT_SELECT:
                {
                    sport_config_t selected_sport = get_sport_config(sport_manager_get_selected_type(&sport_mgr));
                    ui_manager_show_sport_selection(&ui_mgr, &selected_sport, timer_manager_get_seconds(&timer_mgr));
                }
                break;
                
            case INPUT_ACTION_START_STOP:
                ui_manager_update_display(&ui_mgr, &current_sport, timer_manager_get_seconds(&timer_mgr));
                break;
                
            case INPUT_ACTION_NONE:
            default:
                break;
        }

        timer_manager_update(&timer_mgr);

        if (timer_manager_should_send_null(&timer_mgr)) {
            radio_send_time(&radio, 0xFF, 255, 0, 0, sequence++);
            timer_manager_mark_null_sent(&timer_mgr);
        }

        static uint32_t debug_last_timer_output = 0;
        if (time_elapsed(debug_last_timer_output, TIMER_DEBUG_INTERVAL_MS)) {
            ESP_LOGI(TAG, "Timer state - running: %d, seconds: %d", 
                     timer_manager_is_running(&timer_mgr), timer_manager_get_seconds(&timer_mgr));
            debug_last_timer_output = get_current_time_ms();
        }

        if (time_elapsed(radio_last_transmit, RADIO_TRANSMIT_INTERVAL_MS)) {
            uint8_t time_to_send = timer_manager_get_seconds(&timer_mgr);

            if (timer_manager_get_seconds(&timer_mgr) == 0 && !timer_manager_is_running(&timer_mgr)) {
                time_to_send = 0xFF;
            }

            color_t color = get_sport_color(current_sport.color_scheme, time_to_send);
            uint8_t r = color.r, g = color.g, b = color.b;

            radio_send_time(&radio, time_to_send, r, g, b, sequence++);
            radio_last_transmit = get_current_time_ms();
        }

        radio_update_link_status(&radio);

        vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_MS));
    }
}