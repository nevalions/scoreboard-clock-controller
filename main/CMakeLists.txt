idf_component_register(
    SRCS "main.c" "radio_comm.c" "button_driver.c" "../../radio-common/src/radio_common.c"
    INCLUDE_DIRS "../include" "../../radio-common/include"
    REQUIRES driver esp_common esp_driver_gpio esp_driver_spi
)