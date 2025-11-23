# Controller Wiring Guide

## ESP32 Controller to nRF24L01+ Connections

| nRF24L01+ Pin | ESP32 Pin | Function | Description |
|---------------|-----------|----------|-------------|
| VCC | 3.3V | Power | 3.3V power supply |
| GND | GND | Ground | Common ground |
| CE | GPIO5 | Chip Enable | Controls transmit/receive mode |
| CSN | GPIO4 | SPI Chip Select | SPI slave select |
| SCK | GPIO18 | SPI Clock | Serial clock |
| MOSI | GPIO23 | SPI Master Out | Data from ESP32 to nRF24L01+ |
| MISO | GPIO19 | SPI Master In | Data from nRF24L01+ to ESP32 |

## ESP32 Controller to 1602A LCD (I2C) Connections

### I2C Adapter Module Connections
| I2C Adapter Pin | ESP32 Pin | Function | Description |
|-----------------|-----------|----------|-------------|
| VCC | 3.3V | Power | 3.3V power supply |
| GND | GND | Ground | Common ground |
| SDA | GPIO21 | I2C Data | I2C serial data line |
| SCL | GPIO22 | I2C Clock | I2C serial clock line |

### PCF8574T to LCD Module Connections
| PCF8574T Pin | LCD Pin | Function | Description |
|--------------|---------|----------|-------------|
| P0 | RS (4) | Register Select | Command/Data select |
| P1 | RW (5) | Read/Write | Write mode (grounded) |
| P2 | EN (6) | Enable | LCD enable signal |
| P3 | A (15) | Backlight | Backlight control |
| P4 | D4 (11) | Data 4 | 4-bit data line |
| P5 | D5 (12) | Data 5 | 4-bit data line |
| P6 | D6 (13) | Data 6 | 4-bit data line |
| P7 | D7 (14) | Data 7 | 4-bit data line |

### LCD Module Power Connections
| LCD Pin | Connection | Function | Description |
|---------|------------|----------|-------------|
| VSS (1) | GND | Ground | LCD ground |
| VDD (2) | 3.3V | Power | LCD power (3.3V) |
| VO (3) | Potentiometer | Contrast | LCD contrast control |
| K (16) | GND | Backlight Ground | Backlight cathode |

**Note:** The LCD uses a PCF8574T I2C adapter module. Default I2C address is 0x27.

## PCF8574 I2C Adapter Details

### PCF8574 Pin Mapping
The PCF8574 I2C I/O expander converts I2C signals to parallel LCD signals:

| PCF8574 Pin | LCD Signal | Function | Description |
|-------------|------------|----------|-------------|
| P0 | RS | Register Select | 0=Command, 1=Data |
| P1 | RW | Read/Write | 0=Write (grounded), 1=Read |
| P2 | EN | Enable | LCD data latch signal |
| P3 | - | Backlight Control | Optional backlight control |
| P4 | D4 | Data Bit 4 | LCD 4-bit data |
| P5 | D5 | Data Bit 5 | LCD 4-bit data |
| P6 | D6 | Data Bit 6 | LCD 4-bit data |
| P7 | D7 | Data Bit 7 | LCD 4-bit data |

### I2C Address Configuration
The PCF8574 has 3 address pins (A0, A1, A2) that set the I2C address:

| A2 | A1 | A0 | I2C Address | Hex |
|----|----|----|-------------|-----|
| 0  | 0  | 0  | 0x20        | 0x27 (most common) |
| 0  | 0  | 1  | 0x21        | 0x26 |
| 0  | 1  | 0  | 0x22        | 0x25 |
| 0  | 1  | 1  | 0x23        | 0x24 |
| 1  | 0  | 0  | 0x24        | 0x23 |
| 1  | 0  | 1  | 0x25        | 0x22 |
| 1  | 1  | 0  | 0x26        | 0x21 |
| 1  | 1  | 1  | 0x27        | 0x20 |

**Note:** Most modules come with all address pins grounded (0x27 base address + 0x07 offset = 0x27).

### Communication Protocol
1. **4-bit Mode**: LCD operates in 4-bit mode to reduce I2C traffic
2. **Nibble Transfer**: Each LCD byte sent as two 4-bit nibbles
3. **Enable Pulse**: P2 (EN) pulsed high then low to latch data
4. **Backlight Control**: P3 controls LCD backlight (optional)

### Module Variants
- **PCF8574T**: 7-bit address range 0x20-0x27 (your module)
- **PCF8574**: 7-bit address range 0x20-0x27 (DIP package version)
- **PCF8574A**: 7-bit address range 0x38-0x3F (alternative address range)

**PCF8574T Specifics:**
- **Package**: SO-16 surface mount (common on LCD adapter boards)
- **Voltage**: 2.5V to 6V (3.3V compatible)
- **Temperature**: -40°C to +85°C
- **Current**: 200mA max output per pin

## Button Connections

| Button | ESP32 Pin | Function | Description |
|--------|-----------|----------|-------------|
| CONTROL | GPIO0 | Control Button | Single button for start/stop/reset (internal pull-up) |

## Button Wiring

```
Button Wiring:
ESP32 GPIO0 ---- Button ---- GND
               (internal pull-up)
               
When button is pressed: GPIO reads LOW
When button is released: GPIO reads HIGH
```

## Complete Wiring Diagram

```
ESP32              nRF24L01+           I2C Adapter           1602A LCD           Button
------             ----------           ------------           ----------           ------
3.3V -------------- VCC
GND --------------- GND
GPIO5 ------------ CE
GPIO4 ------------ CSN
GPIO18 ----------- SCK
GPIO23 ----------- MOSI
GPIO19 ----------- MISO

3.3V -------------------------------- VCC
GND --------------------------------- GND
GPIO21 ------------------------------ SDA
GPIO22 ------------------------------ SCL

                                        P0 -------------------- RS (4)
                                        P1 -------------------- RW (5)
                                        P2 -------------------- EN (6)
                                        P3 -------------------- A (15)
                                        P4 -------------------- D4 (11)
                                        P5 -------------------- D5 (12)
                                        P6 -------------------- D6 (13)
                                        P7 -------------------- D7 (14)

LCD Module Power:
VSS (1) -------------------------------- GND
VDD (2) -------------------------------- 3.3V
VO (3) --------------------------------- Potentiometer (contrast)
K (16) --------------------------------- GND (backlight)

GPIO0 --------------------------------- Button ---- GND
GPIO2 --------------------------------- STATUS LED (built-in)
```

## Power Requirements

### nRF24L01+ Radio Module
- **Voltage**: 3.3V (do NOT use 5V - will damage the module)
- **Current**: Up to 15mA during transmission
- **Decoupling**: Add 10µF capacitor between VCC and GND near the nRF24L01+ module

### 1602A LCD with I2C Adapter
- **Voltage**: 3.3V (compatible with 5V but 3.3V recommended)
- **Current**: ~1-2mA (backlight adds ~20mA if enabled)
- **I2C Address**: 0x27 (default, may be 0x3F on some modules)
- **Interface**: PCF8574T I2C to parallel converter
- **Protocol**: 4-bit HD44780 over I2C
- **Speed**: 100kHz I2C clock (standard mode)
- **Package**: SO-16 surface mount (typical for LCD adapter boards)

### Total Power Consumption
- **Idle**: ~5mA (radio + LCD)
- **Active**: ~20mA (radio transmitting + LCD + backlight)
- **Recommended Supply**: 500mA+ 3.3V power supply

## Button Specifications

- **Type**: Momentary push button (single button)
- **Wiring**: Active LOW (internal pull-up resistor used)
- **Debounce**: 50ms software debounce implemented
- **Update Rate**: 20Hz (50ms polling interval)
- **Operation**: Press duration detection for different functions

## Operation

### Button Control
1. **Short Press** (< 2 seconds): Toggle start/stop timing
2. **Long Press** (≥ 2 seconds): Reset timer to sport default and stop

### LCD Display
- **Line 1**: Sport name and variation (e.g., "Basketball 24sec")
- **Line 2**: Current time and status (e.g., "Time: 045")
- **Updates**: Real-time display of timer changes and button events

## Radio Configuration

- **Channel**: 76 (2.476 GHz)
- **Data Rate**: 1 Mbps
- **Power**: 0 dBm
- **CRC**: Enabled
- **Auto-ACK**: Enabled
- **Payload Size**: 32 bytes
- **Address**: 0xE7E7E7E7E7

## Troubleshooting

### Button Issues
1. **Button not responding**: Check button connection to GND on GPIO0
2. **Button always pressed**: Verify no short to ground on GPIO0

### Radio Issues  
3. **Radio not transmitting**: Verify 3.3V power and SPI connections
4. **Intermittent operation**: Add 10µF decoupling capacitor
5. **Short range**: Check antenna placement and power supply stability

### LCD Issues
6. **LCD not displaying**: Check I2C connections (SDA/GPIO21, SCL/GPIO22)
7. **LCD shows garbage**: Verify I2C address (try 0x3F if 0x27 doesn't work)
8. **LCD backlight off**: Check 3.3V power connection to LCD module
9. **Wrong I2C address**: Use I2C scanner to find correct address
10. **PCF8574T not responding**: Check address pins (A0-A2) solder bridges
11. **Wrong chip variant**: Verify you have PCF8574T (0x20-0x27) not PCF8574A (0x38-0x3F)
11. **Intermittent LCD**: Add pull-up resistors (4.7kΩ) on SDA/SCL lines

### General Issues
12. **Status LED not working**: Built-in LED on GPIO2 may vary by ESP32 board
13. **System unstable**: Ensure adequate 3.3V power supply (500mA+ recommended)

### I2C Scanner
Use this code to find your LCD I2C address:
```c
#include "driver/i2c.h"

void i2c_scanner() {
    printf("Scanning I2C bus...\n");
    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);
        
        if (ret == ESP_OK) {
            printf("Found device at 0x%02X\n", addr);
        }
    }
}
```