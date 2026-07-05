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
  - Constants: `UPPER_SNAKE_CASE` (`CONTROL_BUTTON_PIN`, `RADIO_STATUS_LED_PIN`)
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
| RADIO_STATUS_LED_PIN | GPIO_NUM_2 | Link quality indicator (defined in radio-common) |
| CONTROL_BUTTON_PIN | GPIO_NUM_0 | Start/stop/reset/menu control (internal pull-up) |
| NRF24_CE_PIN | GPIO_NUM_5 | Radio chip enable |
| NRF24_CSN_PIN | GPIO_NUM_4 | Radio chip select |
| ST7735_CS_PIN | GPIO_NUM_27 | ST7735 SPI chip select |
| ST7735_DC_PIN | GPIO_NUM_26 | ST7735 data/command |
| ST7735_RST_PIN | GPIO_NUM_25 | ST7735 reset |
| ST7735_SDA_PIN | GPIO_NUM_13 | ST7735 SPI MOSI |
| ST7735_SCL_PIN | GPIO_NUM_14 | ST7735 SPI clock |
| ROTARY_CLK_PIN | GPIO_NUM_33 | KY-040 Clock pin (A) |
| ROTARY_DT_PIN | GPIO_NUM_16 | KY-040 Data pin (B) |
| ROTARY_SW_PIN | GPIO_NUM_32 | KY-040 Switch pin (button) |
| BTN_PRESET1_PIN | GPIO_NUM_21 | Preset variant 1 (internal pull-up) |
| BTN_PRESET2_PIN | GPIO_NUM_22 | Preset variant 2 (internal pull-up) |
| BTN_PRESET3_PIN | GPIO_NUM_36 | Preset variant 3 (input-only, **external pull-up required**) |
| BTN_PRESET4_PIN | GPIO_NUM_34 | Preset variant 4 (input-only, **external pull-up required**) |
| BTN_START_PIN | GPIO_NUM_35 | Dedicated start/stop (input-only, **external pull-up required**) |
| BTN_RESET_PIN | GPIO_NUM_15 | Dedicated reset (internal pull-up) |

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

### ST7735 LCD Module (128x160 1.77" TFT) — the only supported display

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
- `rotary_encoder_begin()` explicitly disables internal pull-ups on CLK/DT and relies
  on the KY-040 module's onboard 10kΩ pull-ups; only SW (GPIO32) gets an internal pull-up
- **GPIO34, GPIO35, and GPIO36 are input-only pins with no internal pull-up/pull-down
  capability.** `button_driver.c` only enables the internal pull-up for pins numbered
  below 34 (`if (pin < 34)`), so preset3 (GPIO36), preset4 (GPIO34), and START (GPIO35)
  require external pull-up resistors (e.g. 10kΩ to 3.3V)
- Always verify hardware connections match pin definitions

## Component Architecture

### Main Components
- **main.c**: Application entry point, main control loop coordination, pin definitions, and `apply_current_sport_and_reset()` (stop timer, apply sport, reset countdown, redraw)
- **input_handler.c**: Unified input processing for the control button, rotary encoder, and preset/start/reset buttons
- **sport_manager.c**: Sport selection and configuration management
- **timer_manager.c**: Timer state management, countdown logic, and the 0xFF null-signal delay after reaching zero
- **ui_manager.c**: Public API that forwards to the ST7735 UI modules under `main/ui/`
- **button_driver.c**: Low-level button press detection and debouncing (internal pull-up only for pins < 34)
- **rotary_encoder.c**: KY-040 rotary encoder interface and direction detection
- **radio_comm.c**: nRF24L01+ radio interface, protocol implementation, and link quality monitoring
- **st7735_lcd.c**: ST7735 128x160 TFT LCD driver with SPI interface (only supported display)
- **radio-common/**: Shared radio functionality (submodule) — channel, address, SPI pin, and status LED constants live here
- **sport_selector.c**: Sport configuration management (local component)
- **colors.c**: Per-sport color scheme logic (see Sport Management below)

### Data Flow
1. Button driver detects press events and duration for the control button and the four dedicated preset/start/reset buttons
2. Rotary encoder handles sport-group/variant browsing and confirmation
3. Input handler processes button and rotary encoder events into unified actions
4. Sport manager manages sport selection and configuration
5. Timer manager handles countdown logic, state management, and null-signal timing
6. UI manager updates the ST7735 display with current status
7. Main loop coordinates all components and updates time counter when running
8. Radio comm broadcasts 6-byte time+color packets (4Hz, 3 identical copies per tick) with sequence numbers
9. Status LED (GPIO2) reflects radio link quality in real-time
10. Serial logs provide debugging and status information

### Sport Management
- **Sport Selection**: Rotary encoder browses sport groups; confirming a group enters the
  variant menu, where rotation cycles variants (`>` highlight) and click applies the selection
- **Sport Confirmation**: Selecting a preset/sport/variant always calls `apply_current_sport_and_reset()` — stops the timer, applies the sport, resets the countdown, and redraws. There is no "same sport = quick reset" special case.
- **Default Times**: Each sport has configurable play clock durations
- **Color Schemes** (`colors.c`): Football uses orange `{255,90,0}` normally, deep-orange `{255,40,0}` under `URGENT_COUNTDOWN_THRESHOLD_SEC` (5s), and red `{255,0,0}` at 0 or on the 0xFF null signal. Basketball is always red. Baseball, Volleyball, and Lacrosse are always orange.

### Module Interaction
- **Input Handler**: Processes raw button and encoder events into high-level actions
- **Sport Manager**: Maintains current sport state and handles sport transitions
- **Timer Manager**: Manages countdown state and provides timing services
- **UI Manager**: Public API forwarding to the ST7735 UI modules
  - Clean interface that forwards to specialized ST7735 UI modules
  - Main screen display coordination
  - Sport and variant menu management
  - Time update optimization

### UI Modular Architecture
- **ui/ui_helpers.h/c**: Text formatting, centering, and layout helpers
- **ui/ui_st7735_variant_bar.h/c**: Adaptive variant bar with 1-3 boxes, 4 boxes, or 5+ text layout
- **ui/ui_st7735_main.h/c**: ST7735 main screen rendering with header, underline, variant bar, and timer
- **ui/ui_st7735_menus.h/c**: ST7735 sport selection and variant menu rendering

Benefits:
- **Maintainability**: Each module 100-150 lines with single responsibility
- **Extensibility**: Easy to add new menu styles
- **Testability**: Modules can be unit tested independently
- **Code Reuse**: Helper functions shared across rendering modules
- **Debugging**: Issues isolated to specific modules
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
- **Dependencies**: driver, esp_common, esp_driver_gpio, esp_driver_spi
- **Extra Components**: ../radio-common

### Component Dependencies
```
main/
├── CMakeLists.txt    # Component registration
├── main.c           # Application logic and coordination
├── input_handler.c  # Unified input processing
├── sport_manager.c  # Sport selection management
├── timer_manager.c  # Timer state management
├── ui_manager.c     # UI display management (forwards to main/ui/)
├── button_driver.c  # Button handling and debouncing
├── rotary_encoder.c # Rotary encoder handling and direction detection
├── radio_comm.c     # Radio interface and link monitoring
├── sport_selector.c # Sport configuration management
├── colors.c         # Color definitions for UI
├── font8x8.c        # Font data for ST7735 text rendering
├── st7735_lcd.c      # ST7735 TFT driver and display management
└── ui/               # ST7735 UI rendering modules

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
└── st7735_lcd.h      # ST7735 TFT interface and structures
```

## Display Configuration

### ST7735 TFT (128x160) — only supported display
- **Display Type**: ST7735 1.77" color TFT display
- **Display Size**: 128 pixels × 160 pixels
- **Interface**: SPI (separate bus from radio)
- **Pins**: CS=GPIO27, DC=GPIO26, RST=GPIO25, MOSI=GPIO13, SCK=GPIO14
- **Features**: Color graphics, text rendering, shapes, test patterns

An earlier revision supported a 1602A I2C LCD as a runtime-selectable alternative
(`lcd_i2c.c`, the I2C scanner, and the `USE_ST7735_DISPLAY` toggle in `main.c`). That
code path has been removed; `ui_manager_init_st7735()` is the only display init path.

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
- **GPIO Pull-up Errors**: CLK/DT (GPIO33/16) rely on the KY-040 module's onboard pull-ups
  (internal pull-up is explicitly disabled for these two in `rotary_encoder_begin()`);
  only the SW pin (GPIO32) gets an internal pull-up
- **Rotary Encoder Not Working**: Verify CLK/DT connections and pull-up resistors
- **Preset3/Preset4/START Triggering Randomly**: GPIO34/35/36 are input-only with no
  internal pull-up capability — add external 10kΩ pull-ups to 3.3V
- **Display Not Working**: Verify SPI connections (CS/DC/RST/MOSI/SCK) and 3.3V power to the ST7735

## Protocol Details

### Time + Color Packet Format (6 bytes, fixed payload)
```
[0] Time High:   (seconds >> 8) & 0xFF
[1] Time Low:    seconds & 0xFF
[2] Red Color:   Sport-specific red value (0-255)
[3] Green Color: Sport-specific green value (0-255)
[4] Blue Color:  Sport-specific blue value (0-255)
[5] Sequence:    0-255 (auto-wrapping counter)
```

### Special Values (canonical: radio-common README / radio_config.h)
- **0-99**: whole seconds; **255**: null/clear, broadcast
  `TIMER_NULL_SIGNAL_DELAY_MS` (3000ms) after the timer reaches zero
- **256+d** (d = 0-49): final-countdown deciseconds (tenths, ~10 Hz)
- **Bit 15**: WARN10 flag — 10s buzzer for football (set from `sport_config_t.warn_at_10`)
- **Sequence**: auto-wrapping counter, shared across the 3 burst copies of a tick

### Radio Configuration

> Canonical protocol values live in `radio-common/README.md` +
> `radio-common/include/radio_config.h` — do not restate them here.

- Channel-agile broadcast (boot RPD survey auto-pick + channel menu override),
  250 kbps, fire-and-forget (auto-ACK/auto-retransmit disabled), 3-copy TX burst per tick
- **Link Quality**: success/failure ratio (`RADIO_LINK_SUCCESS_RATE_THRESHOLD` = 0.5f) —
  with no ACKs, "success" means "frame aired", not "receiver heard"
- **Networking**: Plain broadcast — no mesh (no RF24Mesh), node IDs, or route discovery
- **Referee watch uplink**: encrypted ESP-NOW on the WiFi radio (`espnow_watch_rx.c`,
  allowlist in `include/espnow_watches.h`, contract in `radio-common/include/espnow_link.h`)

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
- [ ] ST7735 display shows information correctly
- [ ] Sport selection, preset buttons, and reset function properly
- [ ] Link quality monitoring provides accurate feedback

### Performance Considerations
- Main loop runs at 20Hz (50ms delay)
- Radio updates at 4Hz (250ms intervals), each tick sent as a 3-copy burst
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