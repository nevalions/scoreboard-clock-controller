# ESP32 Scoreboard Clock Controller

ESP32-based controller that transmits real-time data to scoreboard displays via nRF24L01+ radio module.

## Overview

The controller maintains an internal time counter and continuously broadcasts time updates to remote scoreboard displays. It features single-button control for start/stop/reset functionality and provides visual feedback through a status LED.

## Features

- **🎯 Single Button Control**: Intuitive start/stop/reset with press duration detection
- **📡 nRF24L01+ Radio**: Reliable 2.4GHz wireless communication
- **⏱️ Real-time Clock**: Maintains and transmits current time (HH:MM:SS format)
- **🔄 Continuous Updates**: 4Hz update rate (250ms intervals) for smooth display
- **📊 Link Quality Monitoring**: Visual radio link status indicator

## Hardware Requirements

### Components
- ESP32 development board
- nRF24L01+ radio module with PCB antenna
- Momentary push button (normally open)
- 10µF capacitor (optional, recommended for power stability)

### Pin Connections

| nRF24L01+ Pin | ESP32 GPIO | Description |
|-----------------|------------|-------------|
| VCC | 3.3V | Power supply |
| GND | GND | Ground |
| CE | GPIO5 | Chip Enable |
| CSN | GPIO4 | Chip Select (manual control) |
| SCK | GPIO18 | Serial Clock |
| MOSI | GPIO23 | Master Out Slave In |
| MISO | GPIO19 | Master In Slave Out |

| Button | ESP32 GPIO | Description |
|--------|------------|-------------|
| CONTROL | GPIO0 | Start/Stop/Reset (internal pull-up) |

| LED | ESP32 GPIO | Description |
|------|------------|-------------|
| Status | GPIO17 | Link quality indicator |

## Operation

### Button Controls
- **🟢 Short Press** (< 2s): Toggle start/stop timing
- **🔴 Long Press** (≥ 2s): Reset timer to 00:00 and stop

### Status LED Indicators
- **💚 Solid ON**: Good link (>70% success rate, recent activity)
- **⚫ Solid OFF**: No link or poor connection quality  
- **🟡 Blinking**: Recent transmission failures detected

## Installation & Setup

### Prerequisites
- ESP-IDF development environment
- Compatible ESP32 development board
- nRF24L01+ radio module

### Build and Flash
```bash
# Build the project
idf.py build

# Flash to ESP32 and monitor serial
idf.py flash monitor

# Clean build artifacts
idf.py clean

# Configure project settings
idf.py menuconfig
```

## Integration Notes

### Compatible Receivers
- Works with play_clock scoreboard displays
- Receiver must handle 3-byte time-only protocol format
- Time format: HH:MM:SS (24-hour format)

### Expected Behavior
1. Controller starts in stopped state (00:00:00)
2. User presses button to start timing
3. Time increments and broadcasts continuously
4. Receiver displays time and infers control states
5. Link quality monitored via LED

## Troubleshooting

### Common Issues
- **Radio not responding**: Check nRF24L01+ power and connections
- **LED always off**: Verify radio module and antenna connection
- **Time not advancing**: Check button press detection and timing logic
- **Build failures**: Ensure ESP-IDF environment is properly configured

### Getting Help
- Check serial monitor for real-time status and error messages
- Refer to AGENTS.md for development and debugging details
- Verify all pin connections match the hardware configuration