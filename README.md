# ESP32 Controller for Play Clock

This ESP32 controller sends time data to the play clock display via nRF24L01+ radio using a simplified protocol.

## Features

- **Single Control Button**: Start/stop/reset with press duration detection
- **nRF24L01+ Radio**: Transmits time data to play clock
- **Time Tracking**: Maintains internal time counter
- **Continuous Updates**: Sends time updates every 250ms
- **Link Status LED**: Visual indication of radio link quality
- **Enhanced Logging**: Success/failure tracking and periodic status reports

## Hardware Required

- ESP32 development board
- nRF24L01+ radio module
- 1x momentary push button
- Status LED (built-in LED on GPIO17)
- 10µF capacitor (optional, for power stability)

## Pin Connections

### nRF24L01+ Radio
- VCC → 3.3V
- GND → GND
- CE → GPIO5
- CSN → GPIO4
- SCK → GPIO18
- MOSI → GPIO23
- MISO → GPIO19

### Control Button
- CONTROL → GPIO0 (connect to GND, internal pull-up)

### Status LED
- Status LED → GPIO17 (external LED recommended)

## Button Operation

- **Short Press** (< 2 seconds): Toggle start/stop
- **Long Press** (≥ 2 seconds): Reset timer to 00:00 and stop

## Data Protocol

The controller sends continuous 3-byte time packets:
- **Byte 0**: Time high byte (seconds >> 8)
- **Byte 1**: Time low byte (seconds & 0xFF)  
- **Byte 2**: Sequence number (0-255, wraps)

## Build and Flash

```bash
cd controller
idf.py build
idf.py flash monitor
```

## Operation

1. **Short press** the control button to start/pause timing
2. **Long press** the control button to reset to 00:00
3. Time updates are sent continuously every 250ms
4. Receiver infers start/stop/reset from time changes

## Link Status Indication

The status LED provides visual feedback about the radio link quality:
- **LED ON**: Good link (>70% success rate with recent activity)
- **LED OFF**: No link or poor connection
- **LED BLINKING**: Recent transmission failures

Link status is also logged every 10 seconds with success rate statistics.

## Radio Configuration

- Channel: 76 (2.476 GHz)
- Data Rate: 1 Mbps
- Power: 0 dBm
- Address: 0xE7E7E7E7E7
- Auto-ACK: Enabled
- Update Rate: 250ms (4Hz)

## Integration

This controller works with the play_clock receiver. The receiver must be updated to handle the new 3-byte time-only protocol format.