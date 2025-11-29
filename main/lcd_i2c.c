#include "lcd_i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdarg.h>
#include <stdio.h>

static const char *TAG = "LCD_I2C";

static esp_err_t lcd_i2c_write_byte(LcdI2C* lcd, uint8_t data);
static esp_err_t lcd_i2c_write_nibble(LcdI2C* lcd, uint8_t nibble, bool is_data);
static esp_err_t lcd_i2c_pulse_enable(LcdI2C* lcd, uint8_t data);
static void lcd_i2c_write_8bit(LcdI2C* lcd, uint8_t data, bool is_data);

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
    lcd->backlight_state = 0;   // Backlight is fixed ON, not controlled by PCF8574
    
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
        ESP_LOGE(TAG, "Check SDA pin %d and SCL pin %d connections", sda_pin, scl_pin);
        return false;
    }
    ESP_LOGI(TAG, "I2C master bus created successfully");
    
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = lcd->i2c_addr,
        .scl_speed_hz = 100000,
    };
    
    i2c_master_dev_handle_t dev_handle;
    ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus add device failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Check if LCD is connected at address 0x%02X", addr);
        i2c_del_master_bus(bus_handle);
        return false;
    }
    ESP_LOGI(TAG, "I2C device added successfully at address 0x%02X", addr);
    
    lcd->i2c_dev = dev_handle;
    
    vTaskDelay(pdMS_TO_TICKS(50)); // Wait after power-up
    
    // 4-bit initialization sequence for PCF8574 LCD modules
    ESP_LOGI(TAG, "Starting 4-bit LCD initialization");
    lcd_i2c_write_nibble(lcd, 0x03, false);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_i2c_write_nibble(lcd, 0x03, false);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_i2c_write_nibble(lcd, 0x03, false);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_i2c_write_nibble(lcd, 0x02, false); // Switch to 4-bit mode
    vTaskDelay(pdMS_TO_TICKS(5));
    
    // Function set: 4-bit, 2-line, 5x8 dots
    uint8_t func_cmd = LCD_FUNCTION_SET | lcd->display_function;
    ESP_LOGI(TAG, "Sending function set command: 0x%02X", func_cmd);
    lcd_i2c_write_byte(lcd, func_cmd);
    vTaskDelay(pdMS_TO_TICKS(5));
    
    // Display control: display on, cursor off, blink off
    uint8_t disp_cmd = LCD_DISPLAY_CONTROL | lcd->display_control;
    ESP_LOGI(TAG, "Sending display control command: 0x%02X", disp_cmd);
    lcd_i2c_write_byte(lcd, disp_cmd);
    vTaskDelay(pdMS_TO_TICKS(5));
    
    // Clear display
    ESP_LOGI(TAG, "Clearing display");
    lcd_i2c_clear(lcd);
    
    // Entry mode set
    uint8_t entry_cmd = LCD_ENTRY_MODE_SET | lcd->display_mode;
    ESP_LOGI(TAG, "Sending entry mode command: 0x%02X", entry_cmd);
    lcd_i2c_write_byte(lcd, entry_cmd);
    vTaskDelay(pdMS_TO_TICKS(5));
    
    // Test display with visible characters
    ESP_LOGI(TAG, "Testing display with 'TEST LINE 1'");
    lcd_i2c_set_cursor(lcd, 0, 0);
    lcd_i2c_print(lcd, "TEST LINE 1");
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "Testing second line with 'TEST LINE 2'");
    lcd_i2c_set_cursor(lcd, 0, 1);
    lcd_i2c_print(lcd, "TEST LINE 2");
    vTaskDelay(pdMS_TO_TICKS(100));
    
    lcd->initialized = true;
    ESP_LOGI(TAG, "I2C LCD initialized successfully - test text displayed");
    
    return true;
}

static esp_err_t lcd_i2c_write_byte(LcdI2C* lcd, uint8_t data) {
    if (!lcd || !lcd->i2c_dev) {
        ESP_LOGE(TAG, "LCD not initialized for write_byte");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Split byte into two nibbles for 4-bit LCD mode
    esp_err_t ret = ESP_OK;
    ret = lcd_i2c_write_nibble(lcd, data >> 4, false);  // High nibble
    if (ret != ESP_OK) return ret;
    vTaskDelay(pdMS_TO_TICKS(1));
    
    ret = lcd_i2c_write_nibble(lcd, data & 0x0F, false); // Low nibble
    if (ret != ESP_OK) return ret;
    vTaskDelay(pdMS_TO_TICKS(1));
    
    return ESP_OK;
}

static esp_err_t lcd_i2c_write_nibble(LcdI2C* lcd, uint8_t nibble, bool is_data) {
    if (!lcd || !lcd->i2c_dev) {
        ESP_LOGE(TAG, "LCD not initialized for write_nibble");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Map nibble bits to correct PCF8574 pins (based on actual solder mapping)
    uint8_t data = 0;
    if (nibble & 0x01) data |= LCD_D4_MASK; // P4
    if (nibble & 0x02) data |= LCD_D5_MASK; // P3
    if (nibble & 0x04) data |= LCD_D6_MASK; // P2
    if (nibble & 0x08) data |= LCD_D7_MASK; // P1
    
    if (is_data) {
        data |= LCD_RS_MASK; // P7
    }
    
    ESP_LOGD(TAG, "Writing nibble: 0x%01X -> 0x%02X (data=%d)", nibble, data, is_data);
    
    esp_err_t ret = lcd_i2c_pulse_enable(lcd, data);
    vTaskDelay(pdMS_TO_TICKS(1));
    
    return ret;
}

static esp_err_t lcd_i2c_pulse_enable(LcdI2C* lcd, uint8_t data) {
    // Backlight is fixed ON, no need to control it via PCF8574
    uint8_t enable_high = data | LCD_EN_MASK;
    uint8_t enable_low  = data & ~LCD_EN_MASK;
    
    ESP_LOGD(TAG, "Enable pulse: high=0x%02X, low=0x%02X", enable_high, enable_low);
    
    esp_err_t ret = i2c_master_transmit(lcd->i2c_dev, &enable_high, 1, 100); // 100ms timeout
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C enable high failed: %s, data: 0x%02X", esp_err_to_name(ret), enable_high);
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(1));
    
    ret = i2c_master_transmit(lcd->i2c_dev, &enable_low, 1, 100); // 100ms timeout
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C enable low failed: %s, data: 0x%02X", esp_err_to_name(ret), enable_low);
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(1));
    
    return ESP_OK;
}

static void lcd_i2c_write_8bit(LcdI2C* lcd, uint8_t data, bool is_data) {
    uint8_t send_data = data;
    
    if (is_data) {
        send_data |= LCD_RS_MASK;
    }
    
    ESP_LOGD(TAG, "8-bit write: data=0x%02X, is_data=%d, send_data=0x%02X", 
             data, is_data, send_data);
    
    lcd_i2c_pulse_enable(lcd, send_data);
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
    
    // Always use 4-bit mode for PCF8574
    lcd_i2c_write_nibble(lcd, value >> 4, true);  // High nibble
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_i2c_write_nibble(lcd, value & 0x0F, true); // Low nibble
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

void lcd_i2c_test_backlight(LcdI2C* lcd) {
    if (!lcd) return;
    
    ESP_LOGI(TAG, "Testing backlight patterns...");
    
    // Try different backlight configurations
    uint8_t backlight_patterns[] = {0x80, 0x00, 0x01, 0x02, 0x04, 0x08};
    
    for (int i = 0; i < 6; i++) {
        lcd->backlight_state = backlight_patterns[i];
        ESP_LOGI(TAG, "Testing backlight pattern 0x%02X", backlight_patterns[i]);
        
        // Send backlight command
        uint8_t backlight_cmd = backlight_patterns[i];
        i2c_master_transmit(lcd->i2c_dev, &backlight_cmd, 1, -1);
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // Wait 2 seconds to see effect
    }
    
    // Restore default backlight
    lcd->backlight_state = LCD_BACKLIGHT_MASK;
    ESP_LOGI(TAG, "Backlight test completed");
}

// Pin scanning function to determine real PCF8574 to LCD mapping
void lcd_debug_pin_scan(LcdI2C* lcd) {
    if (!lcd || !lcd->i2c_dev) {
        ESP_LOGE(TAG, "LCD not initialized for pin scan");
        return;
    }
    
    ESP_LOGI(TAG, "Starting PCF8574 pin scan - watch LCD for changes");
    ESP_LOGI(TAG, "Each bit will be set HIGH for 1 second, then LOW for 1 second");
    
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t val = (1 << i);
        ESP_LOGI(TAG, "SCAN: Setting bit %d = HIGH (0x%02X)", i, val);
        i2c_master_transmit(lcd->i2c_dev, &val, 1, 1000);
        vTaskDelay(pdMS_TO_TICKS(1000));

        val = 0;
        ESP_LOGI(TAG, "SCAN: Setting bit %d = LOW (0x00)", i);
        i2c_master_transmit(lcd->i2c_dev, &val, 1, 1000);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI(TAG, "Pin scan completed - report what you observed on LCD");
}

// Confirmation test to verify board usability
void lcd_debug_all_pins_test(LcdI2C* lcd) {
    if (!lcd || !lcd->i2c_dev) {
        ESP_LOGE(TAG, "LCD not initialized for all pins test");
        return;
    }
    
    ESP_LOGI(TAG, "Running ALL PINS HIGH/LOW test...");
    ESP_LOGI(TAG, "Setting all pins HIGH (0xFF) for 2 seconds");
    
    uint8_t val = 0xFF;   // all pins high
    i2c_master_transmit(lcd->i2c_dev, &val, 1, 1000);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "Setting all pins LOW (0x00) for 2 seconds");
    val = 0x00;   // all pins low
    i2c_master_transmit(lcd->i2c_dev, &val, 1, 1000);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "All pins test completed");
    ESP_LOGI(TAG, "If nothing changed except backlight → board is unusable");
}