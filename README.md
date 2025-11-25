# ESP32 Scoreboard Clock Controller

ESP32-based controller that transmits real-time sports timing data to scoreboard displays via nRF24L01+ radio module with support for multiple sports and rotary encoder control.

## Overview

The controller maintains an internal time counter and continuously broadcasts time updates to remote scoreboard displays. It features comprehensive sport selection, rotary encoder control for time adjustment, and provides visual feedback through LCD display and status LED.

## Features

- **🎯 Multi-Sport Support**: Basketball, Football, Baseball, Volleyball, Lacrosse with configurable play clocks
- **🎛️ Rotary Encoder Control**: KY-040 encoder for sport selection and time adjustment
- **🔘 Single Button Control**: Intuitive start/stop/reset with press duration detection
- **📡 nRF24L01+ Radio**: Reliable 2.4GHz wireless communication with link quality monitoring
- **⏱️ Real-time Clock**: Maintains and transmits current time with sport-specific configurations
- **🖥️ LCD Display**: 1602A I2C LCD shows current sport, time, and status
- **🔄 Continuous Updates**: 4Hz update rate (250ms intervals) for smooth display
- **📊 Link Quality Monitoring**: Visual radio link status indicator

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
| nRF24L01+ Pin | ESP32 GPIO | Description |
|---------------|------------|-------------|
| VCC | 3.3V | Power supply |
| GND | GND | Ground |
| CE | GPIO5 | Chip Enable |
| CSN | GPIO4 | Chip Select |
| SCK | GPIO18 | Serial Clock |
| MOSI | GPIO23 | Master Out Slave In |
| MISO | GPIO19 | Master In Slave Out |

#### KY-040 Rotary Encoder
| Encoder Pin | ESP32 GPIO | Description |
|-------------|------------|-------------|
| VCC | 3.3V | Power supply |
| GND | GND | Ground |
| CLK | GPIO34 | Clock pin (A) - input-only |
| DT | GPIO35 | Data pin (B) - input-only |
| SW | GPIO32 | Switch pin (button) |

#### LCD Module (1602A with I2C)
| LCD Pin | ESP32 GPIO | Description |
|---------|------------|-------------|
| VCC | 3.3V | Power supply |
| GND | GND | Ground |
| SDA | GPIO21 | I2C Data |
| SCL | GPIO22 | I2C Clock |

#### Control Button and Status LED
| Component | ESP32 GPIO | Description |
|-----------|------------|-------------|
| Control Button | GPIO0 | Start/Stop/Reset (internal pull-up) |
| Status LED | GPIO17 | Link quality indicator |

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
| Sport | Play Clock | Variations |
|-------|------------|------------|
| Basketball | 24s, 30s | Shot clock timing |
| Football | 40s, 25s | Play clock timing |
| Baseball | 14s, 15s, 19s, 20s | Pitch clock timing |
| Volleyball | 8s | Serve timing |
| Lacrosse | 30s | Shot clock timing |

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
- Refer to AGENTS.md for development and debugging details
- Verify all pin connections match hardware configuration
- Ensure proper power supply stability

## Development

For developers contributing to this project, see [AGENTS.md](AGENTS.md) for:
- Development guidelines and coding standards
- Build system details
- Testing procedures
- Architecture documentation