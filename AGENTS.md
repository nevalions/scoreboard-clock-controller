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
- **Modular Architecture**: Separate concerns into distinct modules (input, sport, timer, UI)
- **Interface Design**: Use clear interfaces between modules with well-defined data structures
- **State Management**: Keep state within appropriate manager modules
- **Dependency Injection**: Pass module references as parameters rather than using globals
- **Module Boundaries**: Respect module interfaces and avoid circular dependencies
- **Single Responsibility**: Each module should have one clear purpose
- **State Encapsulation**: Keep module state private, expose only necessary interfaces
- **Thread Safety**: Use struct-based state instead of static variables in functions

### Code Quality Standards
- **Defensive Programming**: Always validate input parameters
- **Resource Management**: Proper initialization and cleanup
- **Error Propagation**: Consistent error handling patterns
- **Documentation**: Clear function and variable naming
- **Maintainability**: Code should be easy to understand and modify
- **Module Boundaries**: Respect module interfaces and avoid circular dependencies
- **State Encapsulation**: Keep module state private and expose only necessary interfaces
- **Single Responsibility**: Each module should have a single, well-defined purpose
- **Thread Safety**: Avoid static variables in functions, use struct members
- **Memory Safety**: Add NULL pointer checks for all pointer parameters
- **Constants**: Replace magic numbers with named constants

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
| ROTARY_CLK_PIN | GPIO_NUM_33 | KY-040 Clock pin (A) |
| ROTARY_DT_PIN | GPIO_NUM_16 | KY-040 Data pin (B) |
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

### ST7735 LCD Module (128x160 1.77" TFT)
| Connection | ESP32 Pin |
|-----------|------------|
| VCC | 3.3V |
| GND | GND |
| CS | GPIO27 |
| DC | GPIO26 |
| RST | GPIO25 |
| SDA/MOSI | GPIO13 |
| SCL/SCK | GPIO14 |

### KY-040 Rotary Encoder Module
| Connection | ESP32 Pin |
|-----------|------------|
| VCC | 3.3V |
| GND | GND |
| CLK | GPIO33 |
| DT | GPIO16 |
| SW | GPIO32 |

**Important Notes**:
- GPIO33 and GPIO16 are standard GPIO pins with full input/output capabilities
- KY-040 module typically includes 10kΩ pull-up resistors
- Internal pull-ups can be enabled for bare encoders
- Always verify hardware connections match pin definitions

## Component Architecture

### Main Components
- **main.c**: Application entry point and main control loop coordination
- **input_handler.c**: Unified input processing for button and rotary encoder events
- **sport_manager.c**: Sport selection and configuration management
- **timer_manager.c**: Timer state management and countdown logic
- **ui_manager.c**: User interface display management and LCD control (supports both I2C LCD and ST7735)
- **button_driver.c**: Low-level button press detection and debouncing
- **rotary_encoder.c**: KY-040 rotary encoder interface and direction detection
- **radio_comm.c**: nRF24L01+ radio interface, protocol implementation, and link quality monitoring
- **lcd_i2c.c**: 1602A LCD driver with I2C/PCF8574 interface
- **st7735_lcd.c**: ST7735 128x160 TFT LCD driver with SPI interface
- **radio-common/**: Shared radio functionality (submodule)
- **sport_selector.c**: Sport configuration management (local component)
- **colors.c**: Color definitions for sport variants and UI elements

### Data Flow
1. Button driver detects press events and duration
2. Rotary encoder handles sport browsing, selection, and time adjustment
3. Input handler processes button and rotary encoder events into unified actions
4. Sport manager manages sport selection and configuration
5. Timer manager handles countdown logic and state management
6. UI manager updates LCD display with current status
7. Main loop coordinates all components and updates time counter when running
8. Radio comm broadcasts time packets (4Hz) with sequence numbers
9. Status LED reflects radio link quality in real-time
10. Serial logs provide debugging and status information

### Sport Management
- **Sport Selection**: Rotary encoder browses sports with preview before confirmation
- **Sport Confirmation**: Rotary encoder button confirms selection or quick resets
- **Time Adjustment**: Button + rotation adjusts time manually
- **Default Times**: Each sport has configurable play clock durations
- **Selection Mode**: LCD shows `>` prefix when browsing sports

### Module Interaction
- **Input Handler**: Processes raw button and encoder events into high-level actions
- **Sport Manager**: Maintains current sport state and handles sport transitions
- **Timer Manager**: Manages countdown state and provides timing services
- **UI Manager**: Renders display based on sport and timer state
- **Main Loop**: Coordinates module updates and handles timing intervals

### Modular Architecture Principles
- **Clear Separation**: Each module handles one specific aspect of functionality
- **Well-Defined Interfaces**: Modules communicate through structured data and function calls
- **State Localization**: Module state is contained within the module and accessed via interfaces
- **Dependency Management**: Modules depend only on interfaces, not implementations
- **Testability**: Each module can be unit tested independently
- **Extensibility**: New features can be added by extending or adding modules
- **Maintainability**: Changes to one module don't affect others if interfaces remain stable

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
├── main.c           # Application logic and coordination
├── input_handler.c  # Unified input processing
├── sport_manager.c  # Sport selection management
├── timer_manager.c  # Timer state management
├── ui_manager.c     # UI display management
├── button_driver.c  # Button handling and debouncing
├── rotary_encoder.c # Rotary encoder handling and direction detection
├── radio_comm.c     # Radio interface and link monitoring
├── sport_selector.c # Sport configuration management
├── colors.c         # Color definitions for UI
└── lcd_i2c.c        # LCD driver and display management

include/
├── input_handler.h  # Input processing interface
├── sport_manager.h  # Sport management interface
├── timer_manager.h  # Timer management interface
├── ui_manager.h     # UI management interface
├── button_driver.h  # Button interface and structures
├── rotary_encoder.h # Rotary encoder interface and structures
├── radio_comm.h     # Radio interface and structures
├── sport_selector.h # Sport selector interface and structures
├── colors.h         # Color definitions and constants
└── lcd_i2c.h        # LCD interface and structures
```

## Display Configuration

### I2C LCD (1602A)
- **Display Type**: 1602A character LCD with PCF8574 I2C adapter
- **Display Size**: 16 characters × 2 lines
- **Interface**: I2C at address 0x27
- **Pins**: SDA=GPIO21, SCL=GPIO22
- **Features**: Character display, backlight control

### ST7735 TFT (128x160)
- **Display Type**: ST7735 1.77" color TFT display
- **Display Size**: 128 pixels × 160 pixels
- **Interface**: SPI (separate bus from radio)
- **Pins**: CS=GPIO27, DC=GPIO26, RST=GPIO25, MOSI=GPIO13, SCK=GPIO14
- **Features**: Color graphics, text rendering, shapes, test patterns
- **Configuration**: Set `USE_ST7735_DISPLAY` in main.c to switch between displays

### Display Selection
To switch between display types, modify the `USE_ST7735_DISPLAY` constant in `main.c`:
```c
#define USE_ST7735_DISPLAY true   // Use ST7735 TFT display
// #define USE_ST7735_DISPLAY false  // Use I2C LCD display
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
- **GPIO Pull-up Errors**: GPIO33/16 support internal pull-ups for bare encoders
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

### Module Development Guidelines
- **Interface First**: Design clear interfaces before implementing module logic
- **State Management**: Use structs to encapsulate module state, avoid static variables
- **Error Handling**: Return bool for success/failure, validate all inputs
- **Logging**: Use ESP_LOG* macros with descriptive TAG for debugging
- **Initialization**: Provide begin/init functions and proper cleanup
- **Dependencies**: Minimize inter-module dependencies, prefer parameter passing
- **Testing**: Design modules to be testable in isolation
- **Documentation**: Use descriptive function and variable names instead of comments