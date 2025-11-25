#include "lcd_i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdarg.h>
#include <stdio.h>

static const char *TAG = "LCD_I2C";

static esp_err_t lcd_i2c_write_byte(LcdI2C* lcd, uint8_t data);
static void lcd_i2c_write_nibble(LcdI2C* lcd, uint8_t nibble, bool is_data);
static void lcd_i2c_pulse_enable(LcdI2C* lcd, uint8_t data);

bool lcd_i2c_begin(LcdI2C* lcd, uint8_t addr, gpio_num_t sda_pin, gpio_num_t scl_pin) {
    if (!lcd) {
        ESP_LOGE(TAG, "LCD pointer is NULL");
        return false;
    }
    
    ESP_LOGI(TAG, "Initializing I2C LCD at address 0x%02X", addr);
    
    lcd->i2c_addr = addr;
    lcd->display_function = LCD_4BIT_MODE | LCD_2LINE | LCD_5x8_DOTS;
    lcd->display_control = LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF;
    lcd->display_mode = LCD_ENTRY_LEFT | LCD_ENTRY_SHIFT_DECREMENT;
    lcd->backlight_state = LCD_BACKLIGHT_MASK;
    
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = scl_pin,
        .sda_io_num = sda_pin,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    i2c_master_bus_handle_t bus_handle;
    esp_err_t ret = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C new master bus failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = lcd->i2c_addr,
        .scl_speed_hz = 100000,
    };
    
    i2c_master_dev_handle_t dev_handle;
    ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus add device failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    lcd->i2c_dev = dev_handle;
    
    vTaskDelay(pdMS_TO_TICKS(50));
    
    lcd_i2c_write_nibble(lcd, 0x03, false);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_i2c_write_nibble(lcd, 0x03, false);
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_i2c_write_nibble(lcd, 0x03, false);
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_i2c_write_nibble(lcd, 0x02, false);
    vTaskDelay(pdMS_TO_TICKS(1));
    
    lcd_i2c_write_byte(lcd, LCD_FUNCTION_SET | lcd->display_function);
    vTaskDelay(pdMS_TO_TICKS(1));
    
    lcd_i2c_write_byte(lcd, LCD_DISPLAY_CONTROL | lcd->display_control);
    vTaskDelay(pdMS_TO_TICKS(1));
    
    lcd_i2c_clear(lcd);
    
    lcd_i2c_write_byte(lcd, LCD_ENTRY_MODE_SET | lcd->display_mode);
    vTaskDelay(pdMS_TO_TICKS(1));
    
    lcd->initialized = true;
    ESP_LOGI(TAG, "I2C LCD initialized successfully");
    
    return true;
}

static esp_err_t lcd_i2c_write_byte(LcdI2C* lcd, uint8_t data) {
    uint8_t write_buf = data | lcd->backlight_state;
    
    esp_err_t ret = i2c_master_transmit(lcd->i2c_dev, &write_buf, 1, -1);
    
    return ret;
}

static void lcd_i2c_write_nibble(LcdI2C* lcd, uint8_t nibble, bool is_data) {
    uint8_t data = nibble << 4;
    
    if (is_data) {
        data |= LCD_RS_MASK;
    }
    
    lcd_i2c_pulse_enable(lcd, data);
    vTaskDelay(pdMS_TO_TICKS(1));
}

static void lcd_i2c_pulse_enable(LcdI2C* lcd, uint8_t data) {
    lcd_i2c_write_byte(lcd, data | LCD_EN_MASK | lcd->backlight_state);
    vTaskDelay(pdMS_TO_TICKS(1));
    
    lcd_i2c_write_byte(lcd, (data & ~LCD_EN_MASK) | lcd->backlight_state);
    vTaskDelay(pdMS_TO_TICKS(1));
}

void lcd_i2c_clear(LcdI2C* lcd) {
    if (!lcd->initialized) return;
    lcd_i2c_write_byte(lcd, LCD_CLEAR_DISPLAY);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_i2c_home(LcdI2C* lcd) {
    if (!lcd->initialized) return;
    lcd_i2c_write_byte(lcd, LCD_RETURN_HOME);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_i2c_set_cursor(LcdI2C* lcd, uint8_t col, uint8_t row) {
    if (!lcd->initialized) return;
    
    uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    if (row >= 2) row = 1;
    if (col >= 16) col = 15;
    
    lcd_i2c_write_byte(lcd, LCD_SET_DDRAM_ADDR | (col + row_offsets[row]));
}

void lcd_i2c_write(LcdI2C* lcd, uint8_t value) {
    if (!lcd->initialized) return;
    
    lcd_i2c_write_nibble(lcd, value >> 4, true);
    vTaskDelay(pdMS_TO_TICKS(1));
    
    lcd_i2c_write_nibble(lcd, value & 0x0F, true);
    vTaskDelay(pdMS_TO_TICKS(1));
}

void lcd_i2c_print(LcdI2C* lcd, const char* str) {
    if (!lcd->initialized) return;
    while (*str) {
        lcd_i2c_write(lcd, *str++);
    }
}

void lcd_i2c_printf(LcdI2C* lcd, const char* format, ...) {
    if (!lcd->initialized) return;
    
    char buffer[64];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    lcd_i2c_print(lcd, buffer);
}