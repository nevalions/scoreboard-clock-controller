# ESP32 Scoreboard Clock Controller

Central controller for a multi-display scoreboard clock system using ESP32 and nRF24L01+ radio modules.

## Overview

This controller manages timing and sport selection for a wireless scoreboard system. It features local controls via button and rotary encoder, while broadcasting time data to multiple remote display units via radio communication.

## Features

- **Sport Selection**: Multiple sport configurations with configurable play clock durations
- **Time Control**: Start, stop, and reset functionality via dedicated buttons
- **Wireless Communication**: nRF24L01+ radio module for broadcasting to display units
- **Local Display**: ST7735 128x160 TFT color display
- **User Interface**: KY-040 rotary encoder plus dedicated preset/start/reset buttons for control
- **Link Quality Monitoring**: Real-time radio link quality indication via status LED

## Hardware Requirements

### Components

- ESP32 development board
- nRF24L01+ radio module with PCB antenna
- KY-040 Rotary Encoder Module
- ST7735 128x160 TFT display
- Control button (normally open, internal pull-up)
- 4x preset buttons, START button, RESET button (momentary, normally open)
- External 10kΩ pull-up resistors for preset3, preset4, and START buttons (GPIO34/35/36 - see note below)
- 10µF capacitor (optional, recommended for power stability)

### Pin Connections

#### nRF24L01+ Radio Module

| nRF24L01+ Pin | ESP32 GPIO | Description         |
| ------------- | ---------- | ------------------- |
| VCC           | 3.3V       | Power supply        |
| GND           | GND        | Ground              |
| CE            | GPIO5      | Chip Enable         |
| CSN           | GPIO4      | Chip Select         |
| SCK           | GPIO18     | Serial Clock        |
| MOSI          | GPIO23     | Master Out Slave In |
| MISO          | GPIO19     | Master In Slave Out |

#### KY-040 Rotary Encoder

| Encoder Pin | ESP32 GPIO | Description                |
| ----------- | ---------- | -------------------------- |
| VCC         | 3.3V       | Power supply               |
| GND         | GND        | Ground                     |
| CLK         | GPIO33     | Clock pin (A)              |
| DT          | GPIO16     | Data pin (B)               |
| SW          | GPIO32     | Switch pin (button)        |

#### Display

**ST7735 128x160 TFT Display**

| TFT Pin | ESP32 GPIO | Description               |
| ------- | ---------- | ------------------------- |
| VCC     | 3.3V       | Power supply              |
| GND     | GND        | Ground                    |
| CS      | GPIO27     | SPI Chip Select           |
| DC      | GPIO26     | Data/Command              |
| RST     | GPIO25     | Reset                     |
| SDA/MOSI| GPIO13     | SPI Data (separate from radio) |
| SCL/SCK | GPIO14     | SPI Clock (separate from radio) |

#### Control Button, Preset/Start/Reset Buttons, and Status LED

| Component       | ESP32 GPIO | Description                              |
| ---------------- | ---------- | ----------------------------------------- |
| Control Button   | GPIO0      | Start/Stop/Reset/Menu (internal pull-up) |
| Preset Button 1  | GPIO21     | Load sport variant preset 1               |
| Preset Button 2  | GPIO22     | Load sport variant preset 2               |
| Preset Button 3  | GPIO36     | Load sport variant preset 3 (input-only)  |
| Preset Button 4  | GPIO34     | Load sport variant preset 4 (input-only)  |
| START Button     | GPIO35     | Dedicated start/stop toggle (input-only)  |
| RESET Button     | GPIO15     | Dedicated reset to sport default          |
| Status LED       | GPIO2      | Link quality indicator                    |

**Important Notes:**

- GPIO33 and GPIO16 are standard GPIO pins with full input/output capabilities
- KY-040 module typically includes 10kΩ pull-up resistors for CLK/DT lines
- If using bare rotary encoder, add external 10kΩ pull-ups to CLK and DT
- ST7735 display uses separate SPI bus from nRF24L01+ radio module (no conflicts)
- Radio uses GPIO18/23 for SPI, ST7735 uses GPIO14/13 for SPI
- **GPIO34, GPIO35, and GPIO36 are input-only pins with NO internal pull-up/pull-down
  capability.** `button_driver.c` only enables the internal pull-up for pins numbered
  below 34, so preset buttons 3 and 4 and the START button **require external pull-up
  resistors (e.g. 10kΩ to 3.3V)**. Without them these buttons will float and trigger
  spuriously.

## Operation

### Button Controls

**Control Button (GPIO0, internal, active while timer is running):**

- **🟢 Short Press** (< 2s): Toggle start/stop timing
- **🔴 Long Press** (≥ 2s): Stop and reset timer to current sport's default time
- **🔄 Double Tap** (< 500ms between presses): Enter sport selection menu

**Dedicated External Buttons:**

- **START** (GPIO35): Toggle start/stop timing (same effect as control button short press). Requires an external pull-up.
- **RESET** (GPIO15): Stop timer and reset to current sport's default time.
- **Preset 1-4** (GPIO21, GPIO22, GPIO36, GPIO34): Immediately stop the timer, load that variant of the current sport group, and reset. Works regardless of current UI state. Presets 3 and 4 require external pull-ups.

### Rotary Encoder Controls

- **Rotation**:
  - **In sport selection menu**: Browse sport groups (Basketball/Football/Baseball/Volleyball/Lacrosse)
    - **Clockwise**: Next sport group
    - **Counter-clockwise**: Previous sport group
  - **While timer is running**: Any rotation opens the sport selection menu (equivalent to double-tapping the control button)
  - **In variant selection menu**: Cycles the variants of the selected group; the `>` marker highlights the current choice
- **Button Press (SW)**:
  - **In sport selection menu**: Confirms the highlighted sport group and enters its variant menu
  - **In variant selection menu**: Confirms the highlighted variant, which stops the timer, resets the countdown, and returns to the running display
  - **While timer is running**: Cycles the TX brightness profile — 100% (day) → 50% (dusk) → 25% (night). The active percentage shows in the TFT status row; the scaled RGB is carried in the radio frame, so remote displays dim without any receiver change

There is no "same sport = quick reset" special case — selecting a sport/variant/preset always stops the timer, applies the sport, resets the countdown, and redraws the display.

### Supported Sports

| Sport      | Play Clock         | Variations         |
| ---------- | ------------------ | ------------------ |
| Basketball | 24s, 30s           | Shot clock timing  |
| Football   | 40s, 25s           | Play clock timing  |
| Baseball   | 14s, 15s, 19s, 20s | Pitch clock timing |
| Volleyball | 8s                 | Serve timing       |
| Lacrosse   | 30s                | Shot clock timing  |

### Status LED Indicators (GPIO2)

- **💚 Solid ON**: Good link (>50% success rate, recent activity)
- **⚫ Solid OFF**: No link or poor connection quality
- **🟡 Blinking**: Recent transmission failures detected

### Display Information

**ST7735 TFT Display:**
- **Color graphics** with sport name, time, and visual indicators
- **Selection highlighting** with colored borders
- **Enhanced visibility** with larger text and status indicators

**Display Modes:**
- **Normal**: Shows current active sport and time
- **Selection**: Browsing mode with visual selection indicators

## Installation & Setup

### Prerequisites

- ESP-IDF v6.1 development environment
- Compatible ESP32 development board
- All required hardware components

### Build and Flash

```bash
# Clone repository
git clone <repository-url>
cd scoreboard_clock/controller

# Build project
idf.py build

# Flash to ESP32 and monitor serial
idf.py flash monitor

# Clean build artifacts
idf.py clean

# Configure project settings
idf.py menuconfig
```

### Configuration Options

Use `idf.py menuconfig` to access:

- Serial flasher configuration
- Component-specific settings
- Bootloader options

## Integration Notes

### Compatible Receivers

- Works with play_clock scoreboard displays
- Receiver must handle 6-byte time + color protocol format
- Time format: Sport-specific timing (0-999 seconds)
- Color format: RGB values (0-255 each) for sport-specific display colors
- Special value 0xFF indicates null/clear display signal

### Expected Behavior

1. Controller boots directly into the sport selection menu (default group/variant is basketball 24-second shot clock)
2. User rotates the encoder to browse sport groups, presses the encoder to preview/confirm a variant, or uses a preset button to jump straight to a specific variant
3. Control button, START, and RESET buttons control timing (start/stop/reset)
4. Time and color broadcast continuously (every 250ms) when a sport is active
5. After reaching zero, the controller keeps broadcasting the elapsed value, then 3 seconds later broadcasts the 0xFF null signal so displays clear
6. Link quality monitored via LED (GPIO2) and serial output

### Protocol Details

#### Time + Color Packet Format (6 bytes, fixed payload)

```
[0] Time High:   (seconds >> 8) & 0xFF
[1] Time Low:    seconds & 0xFF
[2] Red Color:   Sport-specific red value (0-255)
[3] Green Color: Sport-specific green value (0-255)
[4] Blue Color:  Sport-specific blue value (0-255)
[5] Sequence:    0-255 (auto-wrapping counter)
```

#### Radio Configuration

- **Channel**: 76 (2.476 GHz)
- **Data Rate**: 250 kbps
- **Power Level**: 0 dBm
- **Device Address**: 0xE7E7E7E7E7
- **Payload**: Fixed 6-byte payload (no dynamic payloads)
- **CRC**: 1-byte CRC enabled
- **Auto-ACK**: Enabled on pipe 0 for reliability
- **Auto-Retransmit**: SETUP_RETR = 0x4F (1250µs delay, up to 15 retries)
- **Update Rate**: 250ms intervals (4Hz), 3 identical copies per tick (burst redundancy)

This is plain point-to-point nRF24L01+ communication — there is no mesh networking, node IDs, or route discovery (no RF24Mesh).

## Troubleshooting

### Common Issues

- **Radio not responding**: Check nRF24L01+ power and connections
- **LED always off**: Verify radio module and antenna connection
- **Time not advancing**: Check button press detection and timing logic
- **Rotary encoder not working**: Verify CLK/DT connections and pull-up resistors
- **Sport selection not working**: Check rotary encoder button press detection
- **Preset 3/4 or START button trigger randomly or don't register**: GPIO34/35/36 have no
  internal pull-up — verify an external 10kΩ pull-up to 3.3V is installed on each of these pins
- **Display not working**: Verify SPI connections and 3.3V power to the ST7735
- **Build failures**: Ensure ESP-IDF environment is properly configured

### Debug Information

- Serial monitor provides real-time status and error messages
- GPIO debug output shows raw button states
- Radio link quality logged every 10 seconds
- Timer state logged every 5 seconds

### Getting Help

- Check serial monitor for detailed status information
- Verify all pin connections match hardware configuration
- Ensure proper power supply stability

## Architecture

### Modular Design

The controller features a clean modular architecture with clear separation of concerns and well-defined interfaces:

#### Core Management Modules
- **Input Handler**: Centralizes all user input processing from button and rotary encoder events into unified actions
- **Sport Manager**: Manages sport selection, configuration state, and sport transitions
- **Timer Manager**: Handles countdown logic, timer state management, and timing services
- **UI Manager**: Public API forwarding to the ST7735 UI modules
  - Clean interface that forwards to specialized ST7735 UI modules (`main/ui/`)
  - Main screen display coordination
  - Sport and variant menu management
  - Time update optimization

#### Hardware Interface Modules
- **Button Driver**: Low-level button press detection, debouncing, and duration tracking
- **Rotary Encoder**: KY-040 rotary encoder interface with direction detection and button handling
- **Radio Comm**: nRF24L01+ radio interface, protocol implementation, and real-time link quality monitoring
- **ST7735 LCD**: 128x160 TFT display driver with SPI interface and color graphics support (the only display supported — the earlier 1602A I2C LCD driver has been removed)

#### Design Benefits
- **Separation of Concerns**: Each module has a single, well-defined responsibility
- **Clear Interfaces**: Well-defined data structures and function signatures between modules
- **State Encapsulation**: Module state is kept private with controlled access through interfaces
- **Dependency Injection**: Module references passed as parameters rather than using globals
- **Thread Safety**: Struct-based state management avoids static variables in functions
- **Maintainability**: Easy to understand, modify, and extend individual components
- **Testability**: Each module can be tested independently with clear inputs/outputs

#### Data Flow Architecture
1. **Hardware Layer**: Button driver and rotary encoder capture raw input events
2. **Input Processing**: Input handler converts raw events into high-level actions
3. **State Management**: Sport and timer managers maintain application state
4. **UI Updates**: UI manager renders current state to LCD display
5. **Communication**: Radio comm broadcasts time data and monitors link quality
6. **Coordination**: Main loop orchestrates all modules with proper timing

This modular architecture enables easier feature additions, debugging, and maintenance while ensuring clean code organization and minimal coupling between components.

## Development

### Project Structure

```
controller/
├── main/                    # Main application code
│   ├── main.c              # Application entry point and main control loop coordination
│   ├── input_handler.c     # Unified input processing from buttons and rotary encoder
│   ├── sport_manager.c     # Sport selection, configuration, and state management
│   ├── timer_manager.c     # Timer countdown logic and state management
│   ├── ui_manager.c        # UI public API forwarding to ST7735 UI modules
│   ├── button_driver.c     # Low-level button press detection and debouncing
│   ├── rotary_encoder.c    # KY-040 rotary encoder interface and direction detection
│   ├── radio_comm.c        # nRF24L01+ radio interface and link quality monitoring
│   ├── st7735_lcd.c        # ST7735 TFT display driver with SPI interface
│   ├── sport_selector.c    # Sport configuration management and selection logic
│   ├── colors.c            # Color definitions for sport variants and UI
│   ├── font8x8.c           # Font data for ST7735 text rendering
│   └── ui/                 # ST7735 UI rendering modules (helpers, main screen, menus, variant bar)
├── include/                # Header files with module interfaces
│   ├── input_handler.h     # Input processing interface and data structures
│   ├── sport_manager.h     # Sport management interface and configuration types
│   ├── timer_manager.h     # Timer management interface and state definitions
│   ├── ui_manager.h        # UI management interface and display constants
│   ├── button_driver.h     # Button interface and event structures
│   ├── rotary_encoder.h    # Rotary encoder interface and direction enums
│   ├── radio_comm.h        # Radio interface and protocol definitions
│   ├── st7735_lcd.h        # ST7735 TFT interface and SPI communication constants
│   ├── sport_selector.h    # Sport selector interface and configuration structures
│   └── colors.h            # Color definitions and constants for UI elements
├── radio-common/           # Shared radio functionality (submodule)
├── CMakeLists.txt          # Root build configuration
├── main/CMakeLists.txt     # Main component build configuration
├── sdkconfig.defaults      # Default ESP-IDF configuration settings
├── WIRING.md               # Detailed hardware wiring documentation
├── AGENTS.md               # Development guidelines for agents/LLMs
└── README.md               # This file
```

#### Module Organization
- **Core Modules** (`input_handler`, `sport_manager`, `timer_manager`, `ui_manager`): Handle application logic and state
- **Driver Modules** (`button_driver`, `rotary_encoder`, `radio_comm`, `st7735_lcd`): Interface with hardware components
- **Sport Modules** (`sport_selector`, `colors`): Sport configuration management and UI color definitions
- **Interface Headers**: Define clear contracts between modules with well-structured data types
- **Submodules**: Reusable components shared across projects (`radio-common`)

### Build Commands

```bash
# Clean build
idf.py clean

# Build only
idf.py build

# Flash and monitor
idf.py flash monitor

# Configuration menu
idf.py menuconfig

# Clean all
idf.py fullclean
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Support

For issues and questions:

- Check the troubleshooting section
- Review debug output via serial monitor
- Verify hardware connections
- Ensure proper ESP-IDF setup
