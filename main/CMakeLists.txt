idf_component_register(
    SRCS "main.c" "radio_comm.c" "button_driver.c"
    INCLUDE_DIRS "../include"
    REQUIRES driver esp_common esp_driver_gpio esp_driver_spi
)