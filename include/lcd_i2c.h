#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c_master.h"

#define LCD_I2C_ADDR         0x27

#define LCD_I2C_SDA_PIN      GPIO_NUM_21
#define LCD_I2C_SCL_PIN      GPIO_NUM_22

// LCD timing constants
#define LCD_POWER_UP_DELAY_MS 50
#define LCD_INIT_DELAY_MS 5
#define LCD_COMMAND_DELAY_MS 2
#define LCD_NIBBLE_DELAY_MS 1
#define LCD_TEST_DELAY_MS 100
#define LCD_BACKLIGHT_TEST_MS 2000

#define LCD_RS_MASK          0x80   // P7
#define LCD_RW_MASK          0x40   // P6  
#define LCD_EN_MASK          0x20   // P5
#define LCD_BACKLIGHT_MASK   0x00   // Backlight is fixed ON, not controlled by PCF8574
#define LCD_D4_MASK          0x10   // P4
#define LCD_D5_MASK          0x08   // P3
#define LCD_D6_MASK          0x04   // P2
#define LCD_D7_MASK          0x02   // P1

#define LCD_CLEAR_DISPLAY       0x01
#define LCD_RETURN_HOME         0x02
#define LCD_ENTRY_MODE_SET      0x04
#define LCD_DISPLAY_CONTROL     0x08
#define LCD_CURSOR_SHIFT        0x10
#define LCD_FUNCTION_SET        0x20
#define LCD_SET_CGRAM_ADDR      0x40
#define LCD_SET_DDRAM_ADDR      0x80

#define LCD_DISPLAY_ON          0x04
#define LCD_DISPLAY_OFF         0x00
#define LCD_CURSOR_ON           0x02
#define LCD_CURSOR_OFF          0x00
#define LCD_BLINK_ON            0x01
#define LCD_BLINK_OFF           0x00

#define LCD_ENTRY_RIGHT         0x00
#define LCD_ENTRY_LEFT          0x02
#define LCD_ENTRY_SHIFT_INCREMENT 0x01
#define LCD_ENTRY_SHIFT_DECREMENT 0x00

#define LCD_8BIT_MODE           0x10
#define LCD_4BIT_MODE           0x00
#define LCD_2LINE               0x08
#define LCD_1LINE               0x00
#define LCD_5x10_DOTS           0x04
#define LCD_5x8_DOTS            0x00

#define LCD_DISPLAY_MOVE        0x08
#define LCD_CURSOR_MOVE         0x00
#define LCD_MOVE_RIGHT          0x04
#define LCD_MOVE_LEFT           0x00

typedef struct {
    bool initialized;
    uint8_t i2c_addr;
    i2c_master_dev_handle_t i2c_dev;
    uint8_t display_function;
    uint8_t display_control;
    uint8_t display_mode;
    uint8_t backlight_state;
} LcdI2C;

bool lcd_i2c_begin(LcdI2C* lcd, uint8_t addr, gpio_num_t sda_pin, gpio_num_t scl_pin);
void lcd_i2c_clear(LcdI2C* lcd);
void lcd_i2c_home(LcdI2C* lcd);
void lcd_i2c_set_cursor(LcdI2C* lcd, uint8_t col, uint8_t row);
void lcd_i2c_write(LcdI2C* lcd, uint8_t value);
void lcd_i2c_print(LcdI2C* lcd, const char* str);
void lcd_i2c_printf(LcdI2C* lcd, const char* format, ...);

// I2C debugging function
void i2c_scan(gpio_num_t sda_pin, gpio_num_t scl_pin);

// LCD testing function
void lcd_i2c_test_backlight(LcdI2C* lcd);

// Pin scanning function to determine real PCF8574 to LCD mapping
void lcd_debug_pin_scan(LcdI2C* lcd);

// Confirmation test to verify board usability
void lcd_debug_all_pins_test(LcdI2C* lcd);