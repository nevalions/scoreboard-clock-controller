idf_component_register(
    SRCS "main.c" "radio_comm.c" "button_driver.c" "lcd_i2c.c" "st7735_lcd.c" "sport_selector.c" "colors.c" "font8x8.c"  "rotary_encoder.c" "i2c_scanner.c" "sport_manager.c" "timer_manager.c" "ui_manager.c" "input_handler.c" "../../radio-common/src/radio_common.c" 
    INCLUDE_DIRS "../include" "../../radio-common/include"
    REQUIRES driver esp_common esp_driver_gpio esp_driver_spi esp_driver_i2c
)
