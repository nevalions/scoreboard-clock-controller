# ESP32 Controller Development Guidelines

## Quick Start
```bash
# Build project
idf.py build

# Flash and monitor (users only)
idf.py flash monitor

# Clean build
idf.py clean

# Configure settings
idf.py menuconfig
```

## Development Rules
- **🚫 AGENTS/LLMS**: Build only, no flashing
- **✅ Users**: Can build and flash normally
- **📝 Documentation**: Update README.md for user-facing changes

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

## Component Architecture

### Main Components
- **main.c**: Application entry point and main control loop
- **button_driver.c**: Button press detection and duration timing
- **radio_comm.c**: nRF24L01+ radio interface and protocol
- **radio-common/**: Shared radio functionality (submodule)

### Data Flow
1. Button driver detects press events
2. Main loop updates time counter when running
3. Radio comm broadcasts time packets (4Hz)
4. Status LED reflects link quality
5. Logs provide debugging and status information

## Build System

### ESP-IDF Integration
- **Target**: ESP32
- **Framework**: ESP-IDF v6.1
- **Dependencies**: driver, esp_driver_gpio, esp_driver_spi
- **Extra Components**: ../radio-common

### Component Dependencies
```
main/
├── CMakeLists.txt    # Component registration
├── main.c           # Application logic
├── button_driver.c   # Button handling
└── radio_comm.c      # Radio interface

include/
├── button_driver.h   # Button interface
└── radio_comm.h      # Radio interface
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
- Time transmission tracking

### Common Issues
- **SPI Handle NULL**: Check radio-common integration
- **Radio Not Responding**: Verify power and pin connections
- **Compilation Errors**: Check include paths and component dependencies
- **Link Failures**: Ensure all sources are properly linked

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