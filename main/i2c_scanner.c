#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"

static const char *TAG = "I2C_SCANNER";

void i2c_scan(gpio_num_t sda_pin, gpio_num_t scl_pin) {
    ESP_LOGI(TAG, "Starting I2C scan on SDA=%d, SCL=%d", sda_pin, scl_pin);
    
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
        return;
    }
    
    ESP_LOGI(TAG, "Scanning I2C addresses...");
    bool found = false;
    
    for (uint8_t addr = 1; addr < 128; addr++) {
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addr,
            .scl_speed_hz = 100000,
        };
        
        i2c_master_dev_handle_t dev_handle;
        ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
        if (ret == ESP_OK) {
            // Try to read a byte to see if device responds
            uint8_t dummy;
            ret = i2c_master_receive(dev_handle, &dummy, 1, 100);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Found device at address: 0x%02X", addr);
                found = true;
            }
            i2c_master_bus_rm_device(dev_handle);
        }
    }
    
    if (!found) {
        ESP_LOGW(TAG, "No I2C devices found");
    }
    
    i2c_del_master_bus(bus_handle);
    ESP_LOGI(TAG, "I2C scan complete");
}