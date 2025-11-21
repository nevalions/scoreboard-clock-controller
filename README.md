# ESP32 Controller for Play Clock

This ESP32 controller sends start/stop/reset commands to the play clock display via nRF24L01+ radio.

## Features

- **3 Control Buttons**: START, STOP, RESET
- **nRF24L01+ Radio**: Transmits commands to play clock
- **Time Tracking**: Maintains internal time counter
- **Auto-Updates**: Sends time updates every 10 seconds when running
- **Link Status LED**: Visual indication of radio link quality
- **Enhanced Logging**: Success/failure tracking and periodic status reports

## Hardware Required

- ESP32 development board
- nRF24L01+ radio module
- 3x momentary push buttons
- Status LED (built-in LED on GPIO2)
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

### Control Buttons
- START → GPIO0 (connect to GND)
- STOP → GPIO2 (connect to GND)
- RESET → GPIO15 (connect to GND)

### Status LED
- Status LED → GPIO2 (built-in LED on most ESP32 boards)

## Commands Sent

- **CMD_RUN (1)**: Start clock with current time
- **CMD_STOP (0)**: Stop clock
- **CMD_RESET (2)**: Reset clock to 00:00

## Build and Flash

```bash
cd controller
idf.py build
idf.py flash monitor
```

## Operation

1. Press START to begin timing
2. Press STOP to pause timing
3. Press RESET to reset to 00:00
4. Time updates are sent automatically every 10 seconds when running

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

## Integration

This controller works with the play_clock receiver. Both use the same radio frequency and address for communication.