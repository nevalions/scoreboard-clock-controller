idf_component_register(
    SRCS "main.c" "radio_comm.c" "button_driver.c" "st7735_lcd.c" "sport_selector.c" "colors.c" "font8x8.c"  "rotary_encoder.c" "sport_manager.c" "timer_manager.c" "ui_manager.c" "input_handler.c" "../../radio-common/src/radio_common.c"
          "ui/ui_helpers.c" "ui/ui_st7735_main.c" "ui/ui_st7735_menus.c" "ui/ui_st7735_variant_bar.c"
    INCLUDE_DIRS "../include" "../../radio-common/include"
    REQUIRES driver esp_common esp_driver_gpio esp_driver_spi
)
