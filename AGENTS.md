# ESP32 Controller Development Guidelines

## Development Rules
- **🚫 AGENTS/LLMS**: Build only, no flashing
- **✅ Users**: Can build and flash normally
- **📝 Documentation**: Update README.md for user-facing changes

## Quick Start
```bash
# Build project
idf.py build

# Clean build
idf.py clean

# Configure settings
idf.py menuconfig
```

## Code Standards

### Style Guidelines
- **Language**: C (ESP-IDF framework)
- **Headers**: `#pragma once` include guards
- **Includes**: System headers → local headers (relative paths)
- **Naming Conventions**:
  - Functions: `snake_case` (`button_begin`, `radio_send_time`)
  - Variables: `snake_case` (`control_button`, `current_seconds`) 
  - Constants: `UPPER_SNAKE_CASE` (`CONTROL_BUTTON_PIN`, `STATUS_LED_PIN`)
  - Types: `PascalCase` typedefs (`Button`, `RadioComm`)

### Best Practices
- **Logging**: `ESP_LOGI/ESP_LOGE/ESP_LOGD` with file-level TAG
- **Error Handling**: bool returns, check all return values
- **File Structure**: Headers in `include/`, source in `main/`
- **FreeRTOS**: `vTaskDelay(pdMS_TO_TICKS(ms))` for delays
- **GPIO**: Use `gpio_num_t` enum for all pins

## Hardware Configuration

### Pin Assignments
| Pin | GPIO | Function | Description |
|-----|------|----------|-------------|
| STATUS_LED_PIN | GPIO_NUM_17 | Link quality indicator |
| CONTROL_BUTTON_PIN | GPIO_NUM_0 | Start/stop/reset control |
| NRF24_CE_PIN | GPIO_NUM_5 | Radio chip enable |
| NRF24_CSN_PIN | GPIO_NUM_4 | Radio chip select |
| LCD_I2C_SDA_PIN | GPIO_NUM_21 | LCD I2C SDA |
| LCD_I2C_SCL_PIN | GPIO_NUM_22 | LCD I2C SCL |
| ROTARY_CLK_PIN | GPIO_NUM_34 | KY-040 Clock pin (A) |
| ROTARY_DT_PIN | GPIO_NUM_35 | KY-040 Data pin (B) |
| ROTARY_SW_PIN | GPIO_NUM_32 | KY-040 Switch pin (button) |

### Radio Module (nRF24L01+)
| Connection | ESP32 Pin |
|-----------|------------|
| VCC | 3.3V |
| GND | GND |
| CE | GPIO5 |
| CSN | GPIO4 |
| SCK | GPIO18 |
| MOSI | GPIO23 |
| MISO | GPIO19 |

### LCD Module (1602A with I2C Adapter)
| Connection | ESP32 Pin |
|-----------|------------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |

### KY-040 Rotary Encoder Module
| Connection | ESP32 Pin |
|-----------|------------|
| VCC | 3.3V |
| GND | GND |
| CLK | GPIO34 |
| DT | GPIO35 |
| SW | GPIO32 |

**Note**: GPIO34 and GPIO35 are input-only pins without internal pull-ups. The KY-040 module typically includes 10kΩ pull-up resistors, but if using bare encoder, add external 10kΩ pull-ups to CLK and DT lines.

## Component Architecture

### Main Components
- **main.c**: Application entry point and main control loop
- **button_driver.c**: Button press detection and duration timing
- **rotary_encoder.c**: KY-040 rotary encoder interface and handling
- **radio_comm.c**: nRF24L01+ radio interface and protocol
- **lcd_i2c.c**: 1602A LCD driver with I2C/PCF8574 interface
- **radio-common/**: Shared radio functionality (submodule)

### Data Flow
1. Button driver detects press events
2. Rotary encoder handles sport selection and time adjustment
3. Main loop updates time counter when running
4. Radio comm broadcasts time packets (4Hz)
5. LCD displays current sport, time, and status
6. Status LED reflects link quality
7. Logs provide debugging and status information

## Build System

### ESP-IDF Integration
- **Target**: ESP32
- **Framework**: ESP-IDF v6.1
- **Dependencies**: driver, esp_driver_gpio, esp_driver_spi, esp_driver_i2c
- **Extra Components**: ../radio-common

### Component Dependencies
```
main/
├── CMakeLists.txt    # Component registration
├── main.c           # Application logic
├── button_driver.c   # Button handling
├── rotary_encoder.c  # Rotary encoder handling
├── radio_comm.c      # Radio interface
└── lcd_i2c.c         # LCD driver

include/
├── button_driver.h   # Button interface
├── rotary_encoder.h  # Rotary encoder interface
├── radio_comm.h      # Radio interface
└── lcd_i2c.h         # LCD interface
```

## Testing & Debugging

### Build Verification
- Ensure clean compilation with `idf.py build`
- Check for linker errors and missing symbols
- Verify binary size and flash usage

### Runtime Debugging
- Serial monitor shows real-time status
- Radio link quality indicators
- Button event logging
- Rotary encoder movement logging
- Time transmission tracking

### Common Issues
- **SPI Handle NULL**: Check radio-common integration
- **Radio Not Responding**: Verify power and pin connections
- **Compilation Errors**: Check include paths and component dependencies
- **Link Failures**: Ensure all sources are properly linked
- **SPI Flash Warnings**: Enable `SPI_FLASH_SUPPORT_BOYA_CHIP` in menuconfig for Boya flash chips
- **GPIO Pull-up Errors**: GPIO34/35 are input-only, require external pull-ups if not using KY-040 module

## Protocol Details

### Time Packet Format (3 bytes)
```
[0] Time High: (seconds >> 8) & 0xFF
[1] Time Low:  seconds & 0xFF  
[2] Sequence:   0-255 (auto-wrapping counter)
```

### Radio Configuration
- **Channel**: 76 (2.476 GHz)
- **Data Rate**: 1 Mbps
- **Power Level**: 0 dBm
- **Device Address**: 0xE7E7E7E7E7
- **Auto-ACK**: Enabled for reliability
- **Update Rate**: 250ms intervals (4Hz)