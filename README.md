# ESP32 Scoreboard Clock Controller

Central controller for a multi-display scoreboard clock system using ESP32 and nRF24L01+ radio modules.

## Overview

This controller manages timing and sport selection for a wireless scoreboard system. It features local controls via button and rotary encoder, while broadcasting time data to multiple remote display units via radio communication.

## Features

- **Sport Selection**: Multiple sport configurations with configurable play clock durations
- **Time Control**: Start, stop, and reset functionality with manual time adjustment
- **Wireless Communication**: nRF24L01+ radio module for broadcasting to display units
- **Local Display**: Dual display support - 1602A I2C LCD or ST7735 TFT color display
- **User Interface**: KY-040 rotary encoder for navigation and control button for operations
- **Link Quality Monitoring**: Real-time radio link quality indication via status LED

## Hardware Requirements

### Components

- ESP32 development board
- nRF24L01+ radio module with PCB antenna
- KY-040 Rotary Encoder Module
- Display option: 1602A LCD with I2C adapter (PCF8574) OR ST7735 128x160 TFT display
- Momentary push button (normally open)
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

#### Display Options

**Option 1: 1602A LCD with I2C (Default)**

| LCD Pin | ESP32 GPIO | Description  |
| ------- | ---------- | ------------ |
| VCC     | 3.3V       | Power supply |
| GND     | GND        | Ground       |
| SDA     | GPIO21     | I2C Data     |
| SCL     | GPIO22     | I2C Clock    |

**Option 2: ST7735 128x160 TFT Display**

| TFT Pin | ESP32 GPIO | Description               |
| ------- | ---------- | ------------------------- |
| VCC     | 3.3V       | Power supply              |
| GND     | GND        | Ground                    |
| CS      | GPIO27     | SPI Chip Select           |
| DC      | GPIO26     | Data/Command              |
| RST     | GPIO25     | Reset                     |
| SDA/MOSI| GPIO13     | SPI Data (separate from radio) |
| SCL/SCK | GPIO14     | SPI Clock (separate from radio) |

**Display Selection**: Configure in `main.c` by setting `USE_ST7735_DISPLAY` constant.

#### Control Button and Status LED

| Component      | ESP32 GPIO | Description                         |
| -------------- | ---------- | ----------------------------------- |
| Control Button | GPIO0      | Start/Stop/Reset (internal pull-up) |
| Status LED     | GPIO17     | Link quality indicator              |

**Important Notes:**

- GPIO33 and GPIO16 are standard GPIO pins with full input/output capabilities
- KY-040 module typically includes 10kΩ pull-up resistors for CLK/DT lines
- If using bare rotary encoder, add external 10kΩ pull-ups to CLK and DT
- ST7735 display uses separate SPI bus from nRF24L01+ radio module (no conflicts)
- Radio uses GPIO18/23 for SPI, ST7735 uses GPIO14/13 for SPI

## Operation

### Button Controls

- **🟢 Short Press** (< 2s): Toggle start/stop timing
- **🔴 Long Press** (≥ 2s): Reset timer to current sport's default time and stop
- **🔄 Double Tap**: Enter sport selection menu

### Rotary Encoder Controls

- **Rotation Only**: Browse through available sports (selection mode)
  - **Clockwise**: Next sport in sequence
  - **Counter-clockwise**: Previous sport in sequence
  - **LCD shows**: `>Sport Name` (with `>` prefix indicating selection mode)
- **Button Press**: Confirm sport selection OR quick reset
  - **If different sport selected**: Confirms and changes to selected sport
  - **If same sport selected**: Quick reset to sport's default time
- **Button + Rotation**: Adjust time manually
  - **Clockwise**: Increment time (max 999 seconds)
  - **Counter-clockwise**: Decrement time (min 0 seconds)

### Supported Sports

| Sport      | Play Clock         | Variations         |
| ---------- | ------------------ | ------------------ |
| Basketball | 24s, 30s           | Shot clock timing  |
| Football   | 40s, 25s           | Play clock timing  |
| Baseball   | 14s, 15s, 19s, 20s | Pitch clock timing |
| Volleyball | 8s                 | Serve timing       |
| Lacrosse   | 30s                | Shot clock timing  |

### Status LED Indicators

- **💚 Solid ON**: Good link (>70% success rate, recent activity)
- **⚫ Solid OFF**: No link or poor connection quality
- **🟡 Blinking**: Recent transmission failures detected

### Display Information

**1602A I2C LCD:**
- **Line 1**: Sport name and variation (with `>` prefix in selection mode)
- **Line 2**: Current time in seconds (formatted as 3 digits)

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
- Receiver must handle 3-byte time-only protocol format
- Time format: Sport-specific timing (0-999 seconds)
- Special value 0xFF indicates null/clear display signal

### Expected Behavior

1. Controller starts with basketball 24-second shot clock
2. User can rotate encoder to browse sports (selection mode with `>` indicator)
3. Button controls timing (start/stop/reset)
4. Time broadcasts continuously when running
5. After reaching zero, display clears after 3-second delay
6. Link quality monitored via LED and serial output

### Protocol Details

#### Time Packet Format (3 bytes)

```
[0] Time High: (seconds >> 8) & 0xFF
[1] Time Low:  seconds & 0xFF
[2] Sequence:   0-255 (auto-wrapping counter)
```

#### Radio Configuration

- **Channel**: 76 (2.476 GHz)
- **Data Rate**: 1 Mbps
- **Power Level**: 0 dBm
- **Device Address**: 0xE7E7E7E7E7
- **Auto-ACK**: Enabled for reliability
- **Update Rate**: 250ms intervals (4Hz)

## Troubleshooting

### Common Issues

- **Radio not responding**: Check nRF24L01+ power and connections
- **LED always off**: Verify radio module and antenna connection
- **Time not advancing**: Check button press detection and timing logic
- **Rotary encoder not working**: Verify CLK/DT connections and pull-up resistors
- **Sport selection not working**: Check rotary encoder button press detection
- **Display not working**: 
  - **1602A LCD**: Check I2C connections and address (0x27)
  - **ST7735 TFT**: Verify SPI connections and 3.3V power
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
- **UI Manager**: Controls LCD display rendering and user interface updates based on system state

#### Hardware Interface Modules
- **Button Driver**: Low-level button press detection, debouncing, and duration tracking
- **Rotary Encoder**: KY-040 rotary encoder interface with direction detection and button handling
- **Radio Comm**: nRF24L01+ radio interface, protocol implementation, and real-time link quality monitoring
- **LCD I2C**: 1602A display driver with I2C/PCF8574 communication and display management
- **ST7735 LCD**: 128x160 TFT display driver with SPI interface and color graphics support

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
│   ├── input_handler.c     # Unified input processing from button and rotary encoder
│   ├── sport_manager.c     # Sport selection, configuration, and state management
│   ├── timer_manager.c     # Timer countdown logic and state management
│   ├── ui_manager.c        # LCD display management and user interface rendering
│   ├── button_driver.c     # Low-level button press detection and debouncing
│   ├── rotary_encoder.c    # KY-040 rotary encoder interface and direction detection
│   ├── radio_comm.c        # nRF24L01+ radio interface and link quality monitoring
│   ├── lcd_i2c.c           # 1602A LCD driver with I2C/PCF8574 interface
│   ├── st7735_lcd.c        # ST7735 TFT display driver with SPI interface
│   ├── sport_selector.c    # Sport configuration management and selection logic
│   ├── colors.c            # Color definitions for sport variants and UI
│   └── font8x8.c           # Font data for ST7735 text rendering
├── include/                # Header files with module interfaces
│   ├── input_handler.h     # Input processing interface and data structures
│   ├── sport_manager.h     # Sport management interface and configuration types
│   ├── timer_manager.h     # Timer management interface and state definitions
│   ├── ui_manager.h        # UI management interface and display constants
│   ├── button_driver.h     # Button interface and event structures
│   ├── rotary_encoder.h    # Rotary encoder interface and direction enums
│   ├── radio_comm.h        # Radio interface and protocol definitions
│   ├── lcd_i2c.h           # LCD interface and I2C communication constants
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
- **Driver Modules** (`button_driver`, `rotary_encoder`, `radio_comm`, `lcd_i2c`, `st7735_lcd`): Interface with hardware components
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
