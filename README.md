# ESP32 Scoreboard Clock Controller

Central controller for a multi-display scoreboard clock system using ESP32 and nRF24L01+ radio modules.

## Overview

This controller manages timing and sport selection for a wireless scoreboard system. It features local controls via button and rotary encoder, while broadcasting time data to multiple remote display units via radio communication.

## Features

- **Sport Selection**: Multiple sport configurations with configurable play clock durations
- **Time Control**: Start, stop, and reset functionality with manual time adjustment
- **Wireless Communication**: nRF24L01+ radio module for broadcasting to display units
- **Local Display**: 1602A LCD for current status, time, and sport information
- **User Interface**: KY-040 rotary encoder for navigation and control button for operations
- **Link Quality Monitoring**: Real-time radio link quality indication via status LED

## Hardware Requirements

### Components

- ESP32 development board
- nRF24L01+ radio module with PCB antenna
- KY-040 Rotary Encoder Module
- 1602A LCD with I2C adapter (PCF8574)
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
| CLK         | GPIO34     | Clock pin (A) - input-only |
| DT          | GPIO35     | Data pin (B) - input-only  |
| SW          | GPIO32     | Switch pin (button)        |

#### LCD Module (1602A with I2C)

| LCD Pin | ESP32 GPIO | Description  |
| ------- | ---------- | ------------ |
| VCC     | 3.3V       | Power supply |
| GND     | GND        | Ground       |
| SDA     | GPIO21     | I2C Data     |
| SCL     | GPIO22     | I2C Clock    |

#### Control Button and Status LED

| Component      | ESP32 GPIO | Description                         |
| -------------- | ---------- | ----------------------------------- |
| Control Button | GPIO0      | Start/Stop/Reset (internal pull-up) |
| Status LED     | GPIO17     | Link quality indicator              |

**Important Notes:**

- GPIO34 and GPIO35 are input-only pins without internal pull-ups
- KY-040 module typically includes 10kΩ pull-up resistors for CLK/DT lines
- If using bare rotary encoder, add external 10kΩ pull-ups to CLK and DT

## Operation

### Button Controls

- **🟢 Short Press** (< 2s): Toggle start/stop timing
- **🔴 Long Press** (≥ 2s): Reset timer to current sport's default time and stop

### Rotary Encoder Controls

- **Rotation Only**: Cycle through available sports
  - **Clockwise**: Next sport in sequence
  - **Counter-clockwise**: Previous sport in sequence
- **Button + Rotation**: Adjust time manually
  - **Clockwise**: Increment time (max 999 seconds)
  - **Counter-clockwise**: Decrement time (min 0 seconds)
- **Button Press (standalone)**: Quick reset to current sport's default time

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

### LCD Display Information

- **Line 1**: Current sport name and variation
- **Line 2**: Current time in seconds (formatted as 3 digits)

## Installation & Setup

### Prerequisites

- ESP-IDF v6.1 development environment
- Compatible ESP32 development board
- All required hardware components

### Build and Flash

```bash
# Clone repository with submodules
git clone --recursive <repository-url>
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
2. User can rotate encoder to select different sports
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
- **LCD not displaying**: Check I2C connections and address (0x27)
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

## Development

### Project Structure

```
controller/
├── main/                    # Main application code
│   ├── main.c              # Application entry point and sport management
│   ├── button_driver.c     # Button handling and debouncing
│   ├── rotary_encoder.c    # Rotary encoder interface and direction detection
│   ├── radio_comm.c        # Radio communication and link monitoring
│   └── lcd_i2c.c           # LCD display driver and management
├── include/                # Header files
│   ├── button_driver.h     # Button interface and structures
│   ├── rotary_encoder.h    # Rotary encoder interface and structures
│   ├── radio_comm.h        # Radio interface and structures
│   └── lcd_i2c.h           # LCD interface and structures
├── radio-common/           # Shared radio functionality (submodule)
├── sport-selector/         # Sport configuration management (submodule)
├── CMakeLists.txt          # Build configuration
├── sdkconfig.defaults      # Default ESP-IDF configuration
├── WIRING.md               # Detailed wiring documentation
└── README.md               # This file
```

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

# Clean all (including submodules)
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
