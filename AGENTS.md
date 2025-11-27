# ESP32 Controller Development Guidelines

## Development Rules
- **🚫 AGENTS/LLMS**: Build only, no flashing
- **✅ Users**: Can build and flash normally
- **📝 Documentation**: Update README.md for user-facing changes
- **🚫 AGENTS.md**: Do not add AGENTS.md to README.md or any user-facing documentation

## Quick Start
```bash
# Build project
idf.py build

# Clean build
idf.py clean

# Configure settings
idf.py menuconfig

# Flash and monitor (users only)
idf.py flash monitor
```

## Code Standards

### Style Guidelines
- **Language**: C (ESP-IDF framework v6.1)
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
- **Memory Safety**: Add NULL pointer checks for all pointer parameters
- **Constants**: Replace magic numbers with named constants
- **Thread Safety**: Avoid static variables in functions, use struct members

### Refactoring Guidelines
- **Extract Constants**: Replace magic numbers with descriptive constants
- **Helper Functions**: Create reusable functions for common operations
- **Consolidate Logic**: Combine similar switch statements into helper functions
- **Variable Naming**: Use descriptive names that indicate purpose
- **Code Organization**: Group related variables and functions together

### Code Quality Standards
- **Defensive Programming**: Always validate input parameters
- **Resource Management**: Proper initialization and cleanup
- **Error Propagation**: Consistent error handling patterns
- **Documentation**: Clear function and variable naming
- **Maintainability**: Code should be easy to understand and modify

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
| ROTARY_CLK_PIN | GPIO_NUM_34 | KY-040 Clock pin (A) - input-only |
| ROTARY_DT_PIN | GPIO_NUM_35 | KY-040 Data pin (B) - input-only |
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

**Important Notes**:
- GPIO34 and GPIO35 are input-only pins without internal pull-ups
- KY-040 module typically includes 10kΩ pull-up resistors
- If using bare encoder, add external 10kΩ pull-ups to CLK and DT lines
- Always verify hardware connections match pin definitions

## Component Architecture

### Main Components
- **main.c**: Application entry point, main control loop, and sport management
- **button_driver.c**: Button press detection, debouncing, and duration timing
- **rotary_encoder.c**: KY-040 rotary encoder interface, direction detection, and button handling
- **radio_comm.c**: nRF24L01+ radio interface, protocol implementation, and link quality monitoring
- **lcd_i2c.c**: 1602A LCD driver with I2C/PCF8574 interface
- **radio-common/**: Shared radio functionality (submodule)
- **sport_selector/**: Sport configuration management (submodule)

### Data Flow
1. Button driver detects press events and duration
2. Rotary encoder handles sport selection and time adjustment
3. Main loop updates time counter when running
4. Radio comm broadcasts time packets (4Hz) with sequence numbers
5. LCD displays current sport, time, and status information
6. Status LED reflects radio link quality in real-time
7. Serial logs provide debugging and status information

### Sport Management
- **Sport Selection**: Rotary encoder cycles through available sports
- **Time Adjustment**: Button + rotation adjusts time manually
- **Default Times**: Each sport has configurable play clock durations
- **Quick Reset**: Rotary encoder button resets to sport default

## Build System

### ESP-IDF Integration
- **Target**: ESP32
- **Framework**: ESP-IDF v6.1
- **Dependencies**: driver, esp_driver_gpio, esp_driver_spi, esp_driver_i2c
- **Extra Components**: ../radio-common, ../sport-selector

### Component Dependencies
```
main/
├── CMakeLists.txt    # Component registration
├── main.c           # Application logic and sport management
├── button_driver.c   # Button handling and debouncing
├── rotary_encoder.c  # Rotary encoder handling and direction detection
├── radio_comm.c      # Radio interface and link monitoring
└── lcd_i2c.c         # LCD driver and display management

include/
├── button_driver.h   # Button interface and structures
├── rotary_encoder.h  # Rotary encoder interface and structures
├── radio_comm.h      # Radio interface and structures
└── lcd_i2c.h         # LCD interface and structures
```

## Testing & Debugging

### Build Verification
- Ensure clean compilation with `idf.py build`
- Check for linker errors and missing symbols
- Verify binary size and flash usage
- Validate all constants are properly defined

### Runtime Debugging
- Serial monitor shows real-time status and debugging information
- Radio link quality indicators with success/failure tracking
- Button event logging with state changes
- Rotary encoder movement logging with direction detection
- Time transmission tracking with sequence numbers
- Sport selection logging with configuration details

### Debug Features
- GPIO level monitoring for hardware verification
- Timer state logging every 5 seconds
- Link status logging every 10 seconds
- Radio transmission success/failure tracking
- Sport configuration validation

### Common Issues
- **SPI Handle NULL**: Check radio-common integration and initialization
- **Radio Not Responding**: Verify power, connections, and pin assignments
- **Compilation Errors**: Check include paths and component dependencies
- **Link Failures**: Ensure all sources are properly linked and initialized
- **SPI Flash Warnings**: Enable `SPI_FLASH_SUPPORT_BOYA_CHIP` in menuconfig for Boya flash chips
- **GPIO Pull-up Errors**: GPIO34/35 are input-only, require external pull-ups if not using KY-040 module
- **Rotary Encoder Not Working**: Verify CLK/DT connections and pull-up resistors
- **LCD Not Displaying**: Check I2C connections and address (0x27)

## Protocol Details

### Time Packet Format (3 bytes)
```
[0] Time High: (seconds >> 8) & 0xFF
[1] Time Low:  seconds & 0xFF  
[2] Sequence:   0-255 (auto-wrapping counter)
```

### Special Values
- **0xFF**: Null indicator to clear display (sent 3 seconds after timer reaches zero)
- **0-999**: Valid time values in seconds
- **Sequence**: Auto-wrapping counter for packet tracking

### Radio Configuration
- **Channel**: 76 (2.476 GHz)
- **Data Rate**: 1 Mbps
- **Power Level**: 0 dBm
- **Device Address**: 0xE7E7E7E7E7
- **Auto-ACK**: Enabled for reliability
- **Update Rate**: 250ms intervals (4Hz)
- **Retry Logic**: Automatic retries with max retry limit
- **Link Quality**: Monitored via success/failure ratio

## Development Workflow

### Code Review Checklist
- [ ] All magic numbers replaced with constants
- [ ] NULL pointer checks added for all pointer parameters
- [ ] Static variables moved to structs for thread safety
- [ ] Helper functions extracted for common operations
- [ ] Variable names are descriptive and clear
- [ ] Error handling is consistent and comprehensive
- [ ] Logging is appropriate for debugging
- [ ] Code follows established naming conventions

### Testing Requirements
- [ ] Code compiles without warnings or errors
- [ ] All hardware components initialize properly
- [ ] Button press detection works correctly
- [ ] Rotary encoder responds to rotation and button presses
- [ ] Radio communication establishes and maintains link
- [ ] LCD displays information correctly
- [ ] Sport selection and time adjustment function properly
- [ ] Link quality monitoring provides accurate feedback

### Performance Considerations
- Main loop runs at 20Hz (50ms delay)
- Radio updates at 4Hz (250ms intervals)
- Timer updates at 1Hz (1000ms intervals)
- Link quality monitoring at 0.1Hz (10-second intervals)
- Memory usage should remain stable without leaks
- CPU usage should be minimal during normal operation