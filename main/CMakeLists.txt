idf_component_register(
    SRCS "main.c" "radio_comm.c" "button_driver.c" "lcd_i2c.c" "../../radio-common/src/radio_common.c" "../../sport_selector/main/sport_selector.c"
    INCLUDE_DIRS "../include" "../../radio-common/include" "../../sport_selector/include"
    REQUIRES driver esp_common esp_driver_gpio esp_driver_spi esp_driver_i2c
)