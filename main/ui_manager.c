#include "../include/ui_manager.h"
#include "../include/lcd_i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "UI_MGR";

void ui_manager_init(UiManager *manager, uint8_t i2c_addr, gpio_num_t sda_pin, gpio_num_t scl_pin) {
    ESP_LOGI(TAG, "Initializing LCD at address 0x%02X", i2c_addr);
    if (!lcd_i2c_begin(&manager->lcd, i2c_addr, sda_pin, scl_pin)) {
        ESP_LOGE(TAG, "Failed to initialize I2C LCD");
        return;
    }
}

void ui_manager_update_display(UiManager *manager, const sport_config_t *sport, uint16_t seconds) {
    lcd_i2c_clear(&manager->lcd);
    lcd_i2c_set_cursor(&manager->lcd, 0, 0);
    lcd_i2c_printf(&manager->lcd, "%s %s", sport->name, sport->variation);
    lcd_i2c_set_cursor(&manager->lcd, 0, 1);
    lcd_i2c_printf(&manager->lcd, "Time: %03d", seconds);
}

void ui_manager_show_sport_selection(UiManager *manager, const sport_config_t *selected_sport, uint16_t current_seconds) {
    lcd_i2c_clear(&manager->lcd);
    lcd_i2c_set_cursor(&manager->lcd, 0, 0);
    lcd_i2c_printf(&manager->lcd, ">%s %s", selected_sport->name, selected_sport->variation);
    lcd_i2c_set_cursor(&manager->lcd, 0, 1);
    lcd_i2c_printf(&manager->lcd, "Time: %03d", current_seconds);
}

void ui_manager_clear(UiManager *manager) {
    lcd_i2c_clear(&manager->lcd);
}

void ui_manager_run_lcd_tests(UiManager *manager) {
    ESP_LOGI(TAG, "Running LCD PIN SCAN test!");
    lcd_debug_pin_scan(&manager->lcd);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "Running ALL PINS HIGH/LOW confirmation test...");
    lcd_debug_all_pins_test(&manager->lcd);
}