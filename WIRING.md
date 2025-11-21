# Controller Wiring Guide

## ESP32 Controller to nRF24L01+ Connections

| nRF24L01+ Pin | ESP32 Pin | Function | Description |
|---------------|-----------|----------|-------------|
| VCC | 3.3V | Power | 3.3V power supply |
| GND | GND | Ground | Common ground |
| CE | GPIO5 | Chip Enable | Controls transmit/receive mode |
| CSN | GPIO4 | SPI Chip Select | SPI slave select |
| SCK | GPIO18 | SPI Clock | Serial clock |
| MOSI | GPIO23 | SPI Master Out | Data from ESP32 to nRF24L01+ |
| MISO | GPIO19 | SPI Master In | Data from nRF24L01+ to ESP32 |

## Button Connections

| Button | ESP32 Pin | Function | Description |
|--------|-----------|----------|-------------|
| START | GPIO0 | Start Button | Starts the clock (internal pull-up) |
| STOP | GPIO2 | Stop Button | Stops the clock (internal pull-up) |
| RESET | GPIO15 | Reset Button | Resets clock to 00:00 (internal pull-up) |

## Button Wiring

```
Button Wiring (for each button):
ESP32 GPIO ---- Button ---- GND
              (internal pull-up)
              
When button is pressed: GPIO reads LOW
When button is released: GPIO reads HIGH
```

## Complete Wiring Diagram

```
ESP32              nRF24L01+           Buttons
------             ----------           -------
3.3V -------------- VCC
GND --------------- GND
GPIO5 ------------ CE
GPIO4 ------------ CSN
GPIO18 ----------- SCK
GPIO23 ----------- MOSI
GPIO19 ----------- MISO

GPIO0 ------------ START ---- GND
GPIO2 ------------ STOP  ---- GND  
GPIO15 ----------- RESET ---- GND
```

## Power Requirements

- **Voltage**: 3.3V (do NOT use 5V - will damage the module)
- **Current**: Up to 15mA during transmission
- **Decoupling**: Add 10µF capacitor between VCC and GND near the nRF24L01+ module

## Button Specifications

- **Type**: Momentary push buttons
- **Wiring**: Active LOW (internal pull-up resistors used)
- **Debounce**: 50ms software debounce implemented
- **Update Rate**: 20Hz (50ms polling interval)

## Operation

1. **START Button**: Sends CMD_RUN command, starts time counting
2. **STOP Button**: Sends CMD_STOP command, stops time counting  
3. **RESET Button**: Sends CMD_RESET command, resets time to 00:00

## Radio Configuration

- **Channel**: 76 (2.476 GHz)
- **Data Rate**: 1 Mbps
- **Power**: 0 dBm
- **CRC**: Enabled
- **Auto-ACK**: Enabled
- **Payload Size**: 32 bytes
- **Address**: 0xE7E7E7E7E7

## Troubleshooting

1. **Buttons not responding**: Check button connections to GND
2. **Radio not transmitting**: Verify 3.3V power and SPI connections
3. **Intermittent operation**: Add 10µF decoupling capacitor
4. **Short range**: Check antenna placement and power supply stability